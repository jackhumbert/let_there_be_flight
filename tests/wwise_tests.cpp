// The game-free pieces of the Wwise path (src/red4ext/Audio/Wwise, docs/design/wwise-audio.md step 1): Wwise's name
// hash and our game-object range, the game's axes to Wwise's, the callback setter's byte pattern, the relay's dispatch
// (ours, the game's, nobody's, a loop through another mod) and LTBF's bank against mod.bnk's layout.
#include "check.hpp"

#include "Audio/Wwise/Axes.hpp"
#include "Audio/Wwise/Bank.hpp"
#include "Audio/Wwise/Ids.hpp"
#include "Audio/Wwise/Pattern.hpp"
#include "Audio/Wwise/Relay.hpp"
#include "Audio/Wwise/Sends.hpp"

#include <cstring>
#include <vector>

using namespace Wwise;

TEST(wwise_fnv1_is_wwises_hash) {
  // IDs in the game's mod.bnk: its events, its bank, the "sfx" bus
  CHECK(Fnv1("mod_sfx_2d") == 72893892u);
  CHECK(Fnv1("mod_sfx_street") == 2919923931u);
  CHECK(Fnv1("mod") == 1082004799u);
  CHECK(Fnv1("SFX") == 393239870u);
}

TEST(wwise_object_range) {
  CHECK(ObjectId(1) == 0x4C54424600000001ull);
  CHECK(IsOurs(ObjectId(0xFFFFFFFF)));
  CHECK(!IsOurs(1) && !IsOurs(0x4C54424500000001ull));
  const std::vector<AkGameObjectID> game{1, 2, 0x00000F0012345678ull};
  CHECK(RangeFree(game));
  const std::vector<AkGameObjectID> taken{1, ObjectId(7)};
  CHECK(!RangeFree(taken));
}

namespace {

double Dot(const AkVector &a, const AkVector &b) {
  return double(a.x) * b.x + double(a.y) * b.y + double(a.z) * b.z;
}

void CheckSquare(const AkWorldTransform &t) {
  CHECK_NEAR(Dot(t.front, t.front), 1.0, 1e-5);
  CHECK_NEAR(Dot(t.top, t.top), 1.0, 1e-5);
  CHECK_NEAR(Dot(t.front, t.top), 0.0, 1e-5);
}

} // namespace

TEST(wwise_axes_game_to_wwise) {
  // facing the game's +y (north), up +z: Wwise's +z and +y; the position's y and z swap
  const AkWorldTransform t = ToWwise({10, 20, 30}, {0, 1, 0}, {0, 0, 1});
  CHECK_NEAR(t.front.z, 1.0, 1e-6);
  CHECK_NEAR(t.top.y, 1.0, 1e-6);
  CHECK_NEAR(t.position.x, 10.0, 0);
  CHECK_NEAR(t.position.y, 30.0, 0);
  CHECK_NEAR(t.position.z, 20.0, 0);
  // the spike's right of a listener, (front.z, 0, -front.x), is the game's +x for a car facing north
  CHECK_NEAR(t.front.z, 1.0, 1e-6);
  CHECK_NEAR(-t.front.x, 0.0, 1e-6);
  // a pitched facing and an up not square to it: made square, both unit
  const AkWorldTransform pitched = ToWwise({0, 0, 0}, {1, 0, 1}, {0, 0, 1});
  CheckSquare(pitched);
  CHECK(pitched.top.y > 0.5f);
  // no up given: the world's
  CheckSquare(ToWwise({0, 0, 0}, {3, 4, 0}, {0, 0, 0}));
  // facing straight up with no up: still a valid pair
  CheckSquare(ToWwise({0, 0, 0}, {0, 0, 5}, {0, 0, 0}));
  // nothing at all: the game's +y, up
  const AkWorldTransform none = ToWwise({0, 0, 0}, {0, 0, 0}, {0, 0, 0});
  CheckSquare(none);
  CHECK_NEAR(none.front.z, 1.0, 1e-6);
}

namespace {

// a copy of the setter's bytes at aAt in aCode, its stores' displacements chosen so they land at aGlobals, +8, +16
// (or, with aSpread, 32 bytes apart)
void PutSetter(std::vector<uint8_t> &aCode, size_t aAt, uint64_t aBase, uint64_t aGlobals, uint64_t aSpread = 8) {
  const uint8_t bytes[] = {0x48, 0x85, 0xC9, 0x74, 0x16, 0x48, 0x85, 0xD2, 0x74, 0x11, 0x48, 0x89, 0x0D, 0, 0, 0,
                           0,    0x48, 0x89, 0x15, 0,    0,    0,    0,    0x4C, 0x89, 0x05, 0,    0,    0, 0, 0xC3};
  std::memcpy(aCode.data() + aAt, bytes, sizeof(bytes));
  for (int k = 0; k < 3; ++k) {
    const size_t store = aAt + 10 + 7 * k;
    const int32_t d = int32_t(int64_t(aGlobals + k * aSpread) - int64_t(aBase + store + 7));
    std::memcpy(aCode.data() + store + 3, &d, 4);
  }
}

} // namespace

TEST(wwise_setter_pattern) {
  const uint64_t base = 0x141000000;
  std::vector<uint8_t> code(4096, 0xCC);
  PutSetter(code, 1000, base, 0x1439C4D60);
  // a near miss: the same instructions, stores not side by side
  PutSetter(code, 3000, base, 0x1439C4D60, 32);
  const std::vector<SetterMatch> matches = FindSetters(code, base);
  CHECK(matches.size() == 1);
  CHECK(!matches.empty() && matches[0].offset == 1000);
  CHECK(!matches.empty() && matches[0].globals == 0x1439C4D60);
  // the end of the code: a match cut short is not read past it
  std::vector<uint8_t> cut(code.begin(), code.begin() + 1020);
  CHECK(FindSetters(cut, base).empty());
}

namespace {

struct Ramp : Source {
  uint32_t channels = 1;
  int renders = 0;
  uint32_t Channels() const override { return channels; }
  void Render(float *aData, uint32_t aFrames, uint32_t aChannels) override {
    ++renders;
    for (uint32_t c = 0; c < aChannels; ++c) {
      for (uint32_t i = 0; i < aFrames; ++i) {
        aData[c * aFrames + i] = float(c * 1000 + i);
      }
    }
  }
};

AkPlayingID gameSaw = 0;
int gameExecutes = 0;
void GameExecute(AkPlayingID aId, AkAudioBuffer *aBuffer) {
  gameSaw = aId;
  ++gameExecutes;
  aBuffer->eState = 17;
}
void GameFormat(AkPlayingID, AkAudioFormat &aFormat) {
  aFormat.sampleRate = 44100;
}
float GameGain(AkPlayingID) {
  return 0.5f;
}

// another mod that put itself in front of ours and passes on to ours, as ours pass on to it
void ModExecute(AkPlayingID aId, AkAudioBuffer *aBuffer) {
  Trampolines().execute(aId, aBuffer);
}

AkAudioBuffer Buffer(std::vector<float> &aData, uint32_t aChannels, uint16_t aFrames) {
  aData.assign(size_t(aChannels) * aFrames, -1.0f);
  return {aData.data(), ChannelConfig(aChannels), 0, aFrames, 0};
}

} // namespace

TEST(wwise_relay_dispatch) {
  Relay relay;
  Ramp mono, stereo;
  stereo.channels = 2;
  CHECK(relay.Bind(11, &mono));
  CHECK(relay.Bind(12, &stereo));
  CHECK(!relay.Bind(0, &mono));
  relay.SetRate(48000);

  // before anything is installed: the globals empty, so there is nothing to pass to
  const Callbacks ours{[](AkPlayingID, AkAudioBuffer *) {}, [](AkPlayingID, AkAudioFormat &) {},
                       [](AkPlayingID) { return 1.0f; }};
  CHECK(relay.Observe({}, ours));
  CHECK(relay.Passed().execute == nullptr);

  // ours: the format, then the samples
  AkAudioFormat format{0, 0, 0};
  relay.Format(12, format);
  CHECK(format.sampleRate == 48000 && format.channelConfig == ChannelConfig(2) && format.bits == kFloatBits);
  std::vector<float> data;
  AkAudioBuffer buffer = Buffer(data, 2, 512);
  relay.Execute(12, &buffer);
  CHECK(stereo.renders == 1);
  CHECK(buffer.eState == AK_DataReady && buffer.uValidFrames == 512);
  CHECK_NEAR(data[511], 511.0, 0);
  CHECK_NEAR(data[512 + 3], 1003.0, 0);
  CHECK_NEAR(relay.Gain(11), 1.0, 0);

  // not ours, nobody before us: left alone
  AkAudioBuffer other = Buffer(data, 1, 512);
  relay.Execute(99, &other);
  CHECK(other.eState == 0 && relay.Read().unanswered == 1);

  // ours in, then the game sets its own after ours: noticed once, passed to from then on
  CHECK(!relay.Observe(ours, ours));
  CHECK(relay.Observe({GameExecute, GameFormat, GameGain}, ours));
  CHECK(relay.Passed().execute == GameExecute);
  relay.Execute(99, &other);
  CHECK(gameSaw == 99 && other.eState == 17);
  AkAudioFormat theirs{0, 0, 0};
  relay.Format(99, theirs);
  CHECK(theirs.sampleRate == 44100);
  CHECK_NEAR(relay.Gain(99), 0.5, 0);
  // ours back in: what is passed to stays the game's
  CHECK(!relay.Observe(ours, ours));
  CHECK(relay.Passed().execute == GameExecute);

  // unbound: passed to the game like any other ID
  relay.Unbind(11);
  gameExecutes = 0;
  relay.Execute(11, &other);
  CHECK(gameExecutes == 1 && mono.renders == 0);
  CHECK(relay.Read().ours == 1 && relay.Read().passed == 2);

  // every slot taken
  Relay full;
  for (int i = 0; i < Relay::kSlots; ++i) {
    CHECK(full.Bind(AkPlayingID(100 + i), &mono));
  }
  CHECK(!full.Bind(999, &mono));
  full.Unbind(100);
  CHECK(full.Bind(999, &mono));
}

TEST(wwise_relay_no_loop_through_another_mod) {
  // a mod installed after us that passes to ours: ours pass an unknown ID to it once, and it comes back to us no more
  Relay &relay = TheRelay();
  const Callbacks ours = Trampolines();
  relay.Observe({ModExecute, GameFormat, GameGain}, ours);
  std::vector<float> data;
  AkAudioBuffer buffer = Buffer(data, 1, 64);
  const uint64_t loops = relay.Read().loops;
  relay.Execute(4242, &buffer);
  CHECK(relay.Read().loops == loops + 1);
}

namespace {

uint32_t U32(const std::vector<uint8_t> &aBytes, size_t aAt) {
  uint32_t v;
  std::memcpy(&v, aBytes.data() + aAt, 4);
  return v;
}

} // namespace

TEST(wwise_bank_reads_back) {
  const Bank::Sound sounds[] = {{"ltbf_voice"}, {"ltbf_bed", false}};
  const std::vector<uint8_t> bytes = Bank::Build("ltbf", sounds);
  const Bank::Parsed bank = Bank::Parse(bytes);
  CHECK(bank.ok);
  CHECK(bank.version == 150 && bank.bank == Fnv1("ltbf") && bank.project == Bank::kProject);
  CHECK(bank.language == Fnv1("sfx"));
  CHECK(bank.objects.size() == 8);
  for (const Bank::Sound &sound : sounds) {
    // the event plays the action, the action the sound, the sound's source the Audio Input parameters
    const Bank::Ids ids = Bank::IdsFor(sound.event);
    CHECK(ids.event == Fnv1(sound.event));
    const Bank::Object *event = bank.Find(ids.event), *action = bank.Find(ids.action), *body = bank.Find(ids.sound),
                       *input = bank.Find(ids.input);
    CHECK(event && action && body && input);
    if (!(event && action && body && input)) {
      continue;
    }
    CHECK(event->type == Bank::kEvent && event->body.size() == 5 && U32(event->body, 1) == ids.action);
    CHECK(action->type == Bank::kAction && U32(action->body, 2) == ids.sound && U32(action->body, 10) == bank.bank);
    CHECK(body->type == Bank::kSound && U32(body->body, 0) == Bank::kAudioInput && U32(body->body, 5) == ids.input);
    CHECK(U32(body->body, 22) == Bank::kSfxBus);
    // positioned: spatialised by position and orientation, with no attenuation
    CHECK(body->body[34] == (sound.positioned ? 0x02 : 0x00));
    CHECK(input->type == Bank::kFxCustom && U32(input->body, 0) == Bank::kAudioInput);
  }
  // a short or a padded bank is refused
  CHECK(!Bank::Parse(std::span(bytes).first(bytes.size() - 1)).ok);
  std::vector<uint8_t> broken = bytes;
  broken[4] += 1;
  CHECK(!Bank::Parse(broken).ok);
}

TEST(wwise_bank_layout_is_mod_bnks) {
  // the game's mod.bnk, its mod_sfx_2d Sound (ID 452459865) after the ID: our 2D Sound for the same source and bus
  // must match it but for two bytes, the 3D bits (0x08 there: attenuation on, with no attenuation set) and the aux
  // bits (0x01 here: the game's aux sends overridden, off)
  const uint8_t game[] = {0x02, 0x00, 0xc8, 0x00, 0x00, 0x93, 0xa4, 0xa3, 0x29, 0x00, 0x00, 0x00, 0x00,
                          0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x37, 0xbc, 0xb7, 0xe2,
                          0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0x08, 0x00, 0x00, 0x00, 0x00,
                          0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
  const Bank::Sound sound{"mod_sfx_2d_copy", false, 0xe2b7bc37};
  const Bank::Parsed bank = Bank::Parse(Bank::Build("ltbf", std::span(&sound, 1)));
  const Bank::Object *body = bank.Find(Bank::IdsFor(sound.event).sound);
  CHECK(body && body->body.size() == sizeof(game));
  if (!body || body->body.size() != sizeof(game)) {
    return;
  }
  std::vector<uint8_t> expected(game, game + sizeof(game));
  const AkUniqueID input = Bank::IdsFor(sound.event).input;
  std::memcpy(expected.data() + 5, &input, 4);
  expected[34] = 0x00;
  expected[35] = 0x01;
  CHECK(body->body == expected);
  // the Play action's and the plugin parameters' sizes as mod.bnk's (18 and 19 bytes after the ID)
  CHECK(bank.Find(Bank::IdsFor(sound.event).action)->body.size() == 18);
  CHECK(bank.Find(Bank::IdsFor(sound.event).input)->body.size() == 19);
}

TEST(wwise_bank_copies_mod_sfx_street) {
  // mod.bnk's mod_sfx_street Sound (ID 930278369) after the ID, up to its RTPC list (one RTPC there, none here): its
  // copy in our bank, the same parent, properties, attenuation and aux send, must match it byte for byte but the source
  // ID (in game the two play alike, 0.1 dB apart)
  const uint8_t game[] = {0x02, 0x00, 0xc8, 0x00, 0x00, 0x2f, 0xb3, 0x94, 0x15, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                          0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x3e, 0x5d, 0x70, 0x17, 0x52, 0x12, 0x02, 0x1b,
                          0x00, 0x03, 0x0c, 0x29, 0x55, 0x00, 0x00, 0x80, 0xc1, 0x00, 0x00, 0xc8, 0x42, 0xec, 0x65,
                          0xa6, 0x1b, 0x00, 0x03, 0x0a, 0x0b, 0xc6, 0x68, 0xec, 0x37, 0x00, 0x00, 0x00, 0x00, 0x00,
                          0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00,
                          0x00, 0x00, 0x00, 0x00};
  const Bank::Prop props[] = {{0x0c, 0xc1800000u}, {0x29, 0x42c80000u}, {0x55, 463889900u}};
  const Bank::Sound sound{"street_copy", true, Bank::kSfxBus, 453120594, props, true, 0x37ec68c6};
  const Bank::Parsed bank = Bank::Parse(Bank::Build("ltbf", std::span(&sound, 1)));
  const Bank::Object *body = bank.Find(Bank::IdsFor(sound.event).sound);
  CHECK(body && body->body.size() == sizeof(game) + 2);
  if (!body || body->body.size() != sizeof(game) + 2) {
    return;
  }
  std::vector<uint8_t> expected(game, game + sizeof(game));
  const AkUniqueID input = Bank::IdsFor(sound.event).input;
  std::memcpy(expected.data() + 5, &input, 4);
  CHECK(std::vector<uint8_t>(body->body.begin(), body->body.end() - 2) == expected);
}

TEST(wwise_bank_game_sends) {
  // a sound that takes the game's sends differs from one that does not in its aux byte alone: 0x03 against 0x01
  const Bank::Sound plain[] = {{"ltbf_test_3d", true}};
  const Bank::Sound room[] = {{"ltbf_test_3d", true, Bank::kSfxBus, 0, {}, false, 0, 0, true}};
  const auto a = Bank::Build("ltbf_test", plain), b = Bank::Build("ltbf_test", room);
  CHECK(a.size() == b.size());
  int differ = 0, at = -1;
  for (size_t i = 0; i < a.size() && i < b.size(); ++i)
    if (a[i] != b[i]) {
      ++differ;
      at = int(i);
    }
  CHECK(differ == 1);
  CHECK(at >= 0 && a[size_t(at)] == 0x01 && b[size_t(at)] == 0x03);
  CHECK(Bank::Parse(b).ok);
}

TEST(wwise_sends_book) {
  // the game's objects noted as it sets their sends, ours left out, oldest first, forgotten on request
  Sends::Book book;
  AkAuxSendValue v{kInvalidGameObject, 123, 0.5f};
  book.Note(7, &v, 1);
  book.Note(ObjectId(3), &v, 1);
  book.Note(9, &v, 1);
  book.Note(7, &v, 1);
  const auto objects = book.Objects();
  CHECK(objects.size() == 2 && objects[0] == 9 && objects[1] == 7);
  CHECK(book.TakeCalls() == 3);
  CHECK(book.TakeCalls() == 0);
  book.Forget(9);
  CHECK(book.Objects().size() == 1);
}

TEST(wwise_sends_car_and_listener) {
  // the car's object is the nearest within range; its sends go on ours for our listener; a reverb bus is named
  const std::vector<AkVector64> at = {{10, 0, 0}, {2, 1, 0}, {0, 0, 3}};
  CHECK(Sends::Nearest(at, {0, 0, 0}, 6.0) == 1);
  CHECK(Sends::Nearest(at, {100, 0, 0}, 6.0) == -1);
  Sends::Set s;
  s.values[0] = {kInvalidGameObject, Fnv1("revb_exterior_default__medium"), 0.8f};
  s.count = 1;
  const Sends::Set ours = Sends::ForListener(s, ObjectId(1));
  CHECK(ours.count == 1 && ours.values[0].listenerID == ObjectId(1) && ours.values[0].controlValue == 0.8f);
  CHECK(Sends::Describe(s) == "revb_exterior_default__medium 0.80");
  CHECK(Sends::BusName(42) == "bus 42");
  CHECK(Sends::Describe({}) == "none");
}
