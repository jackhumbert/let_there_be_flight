#include "Bank.hpp"

#include <bit>
#include <cstring>
#include <string>

namespace Wwise::Bank {

namespace {

// the attenuation's property ID on a Sound, and the spread's place among an attenuation's parameters (mod.bnk's: dry,
// game wet, user wet, low-pass, high-pass, spread, focus)
constexpr uint8_t kAttenuationProp = 0x55;
constexpr int kSpreadParam = 5;

struct Writer {
  std::vector<uint8_t> bytes;
  void U8(uint32_t v) { bytes.push_back(uint8_t(v)); }
  void U16(uint32_t v) {
    U8(v);
    U8(v >> 8);
  }
  void U32(uint32_t v) {
    U16(v);
    U16(v >> 16);
  }
  void Zeros(size_t n) { bytes.insert(bytes.end(), n, 0); }
  void Append(const std::vector<uint8_t> &b) { bytes.insert(bytes.end(), b.begin(), b.end()); }
};

void Chunk(Writer &aOut, const char *aTag, const Writer &aBody) {
  aOut.bytes.insert(aOut.bytes.end(), aTag, aTag + 4);
  aOut.U32(uint32_t(aBody.bytes.size()));
  aOut.Append(aBody.bytes);
}

void Put(Writer &aHirc, uint32_t &aCount, Type aType, AkUniqueID aId, const Writer &aBody) {
  aHirc.U8(aType);
  aHirc.U32(uint32_t(4 + aBody.bytes.size()));
  aHirc.U32(aId);
  aHirc.Append(aBody.bytes);
  ++aCount;
}

// the Audio Input plugin's parameters: its gain (0 dB), no bank data, no RTPCs, states or properties
Writer Input() {
  Writer w;
  w.U32(kAudioInput);
  w.U32(4);
  w.U32(0); // the gain, a float 0
  w.U8(0);  // bank data
  w.U16(0); // RTPCs
  w.U8(0);  // state properties
  w.U8(0);  // state groups
  w.U16(0); // property values
  return w;
}

Writer SoundBody(const Sound &aSound, const Ids &aIds) {
  Writer w;
  // the source: the plugin, its data inline (stream type 0), its parameters in the FxCustom of the source's ID
  w.U32(kAudioInput);
  w.U8(0);
  w.U32(aIds.input);
  w.U32(0); // media size
  w.U8(0);  // source bits
  w.U32(0); // inline parameters
  // the node: no effects, no metadata, our bus, no parent
  w.U8(0);
  w.U8(0);
  w.U8(0);
  w.U8(0);
  w.U32(aSound.bus);
  w.U32(aSound.parent);
  w.U8(0); // priority bits
  std::vector<Prop> props(aSound.props.begin(), aSound.props.end());
  if (aSound.spread) {
    props.push_back({kAttenuationProp, aSound.spread});
  }
  w.U8(uint32_t(props.size()));
  for (const Prop &prop : props) {
    w.U8(prop.id);
  }
  for (const Prop &prop : props) {
    w.U32(prop.value);
  }
  w.U8(0); // ranged properties
  // positioning: overridden here, listener-relative routing; 3D bits: spatialised by position and orientation (2),
  // or not at all; the attenuation (8) off unless asked for
  w.U8(0x03);
  w.U8((aSound.positioned ? 0x02 : 0x00) | (aSound.attenuated || aSound.spread ? 0x08 : 0x00));
  // aux: its own sends over the parent's (1), the game-defined ones its object has (2), user sends (8, one, as
  // mod_sfx_street's 0x0b); no reflections bus
  if (aSound.aux) {
    w.U8(0x0b);
    w.U32(aSound.aux);
    w.Zeros(12);
  } else {
    w.U8(aSound.gameSends ? 0x03 : 0x01);
  }
  w.U32(0);
  // advanced: play on from the elapsed time when virtual, no instance limit, keep playing under the threshold
  w.U8(0);
  w.U8(1);
  w.U16(0);
  w.U8(0);
  w.U8(0);
  w.U8(0); // state properties
  w.U8(0); // state groups
  w.U16(0); // RTPCs
  return w;
}

// the attenuation: its first two flags as mod.bnk's (01 00), each of its 19 parameters' curve (0xff none; 0xfe past
// the ones it has, as mod.bnk's), the curves, no RTPCs
Writer SpreadBody(const Spread &aSpread) {
  Writer w;
  w.U8(1);
  w.U8(0);
  for (int k = 0; k < 19; ++k) {
    w.U8(k == kSpreadParam ? 0 : k < 7 ? 0xff : 0xfe);
  }
  w.U8(1);
  w.U8(0); // the spread's scaling: none
  w.U16(uint32_t(aSpread.points.size()));
  for (const CurvePoint &p : aSpread.points) {
    w.U32(std::bit_cast<uint32_t>(p.distance));
    w.U32(std::bit_cast<uint32_t>(p.value));
    w.U32(4); // linear to the next
  }
  w.U16(0);
  return w;
}

Writer PlayBody(AkUniqueID aTarget, AkUniqueID aBank) {
  Writer w;
  w.U16(0x0403); // Play
  w.U32(aTarget);
  w.U8(0);  // the target is not a bus
  w.U8(0);  // properties
  w.U8(0);  // ranged properties
  w.U8(4);  // fade curve: linear
  w.U32(aBank);
  w.U32(0); // the bank's type: a user bank
  return w;
}

struct Reader {
  std::span<const uint8_t> bytes;
  size_t at = 0;
  bool ok = true;
  bool Has(size_t n) { return ok = ok && at + n <= bytes.size(); }
  uint32_t U32() {
    if (!Has(4)) {
      return 0;
    }
    uint32_t v;
    std::memcpy(&v, bytes.data() + at, 4);
    at += 4;
    return v;
  }
  uint8_t U8() { return Has(1) ? bytes[at++] : 0; }
};

} // namespace

Ids IdsFor(std::string_view aEvent) {
  const std::string name(aEvent);
  return {Fnv1(name), Fnv1(name + "/play"), Fnv1(name + "/sound"), Fnv1(name + "/input")};
}

std::vector<uint8_t> Build(std::string_view aBank, std::span<const Sound> aSounds, std::span<const Spread> aSpreads) {
  const AkUniqueID bank = Fnv1(aBank);
  Writer header;
  header.U32(kVersion);
  header.U32(bank);
  header.U32(Fnv1("sfx")); // the language: none, "SFX"
  header.U32(0x10);        // alignment 16
  header.U32(kProject);
  header.U32(0); // the bank's type: a user bank
  header.Zeros(16); // the bank's hash

  Writer objects;
  uint32_t count = 0;
  for (const Spread &spread : aSpreads) {
    Put(objects, count, kAttenuation, Fnv1(spread.name), SpreadBody(spread));
  }
  for (const Sound &sound : aSounds) {
    const Ids ids = IdsFor(sound.event);
    Put(objects, count, kFxCustom, ids.input, Input());
    Put(objects, count, kSound, ids.sound, SoundBody(sound, ids));
    Put(objects, count, kAction, ids.action, PlayBody(ids.sound, bank));
    Writer event;
    event.U8(1);
    event.U32(ids.action);
    Put(objects, count, kEvent, ids.event, event);
  }
  Writer hirc;
  hirc.U32(count);
  hirc.Append(objects.bytes);

  Writer out;
  Chunk(out, "BKHD", header);
  Chunk(out, "HIRC", hirc);
  return out.bytes;
}

const Object *Parsed::Find(AkUniqueID aId) const {
  for (const Object &o : objects) {
    if (o.id == aId) {
      return &o;
    }
  }
  return nullptr;
}

Parsed Parse(std::span<const uint8_t> aBytes) {
  Parsed out;
  Reader r{aBytes};
  bool header = false;
  while (r.ok && r.at < aBytes.size()) {
    if (!r.Has(8)) {
      return out;
    }
    const std::string tag(reinterpret_cast<const char *>(aBytes.data() + r.at), 4);
    r.at += 4;
    const uint32_t size = r.U32();
    if (!r.Has(size)) {
      return out;
    }
    Reader chunk{aBytes.subspan(r.at, size)};
    r.at += size;
    if (tag == "BKHD") {
      out.version = chunk.U32();
      out.bank = chunk.U32();
      out.language = chunk.U32();
      chunk.U32();
      out.project = chunk.U32();
      header = chunk.ok;
    } else if (tag == "HIRC") {
      const uint32_t count = chunk.U32();
      for (uint32_t i = 0; i < count && chunk.ok; ++i) {
        Object o;
        o.type = chunk.U8();
        const uint32_t n = chunk.U32();
        o.id = chunk.U32();
        if (n < 4 || !chunk.Has(n - 4)) {
          return out;
        }
        o.body.assign(chunk.bytes.begin() + chunk.at, chunk.bytes.begin() + chunk.at + (n - 4));
        chunk.at += n - 4;
        out.objects.push_back(std::move(o));
      }
      if (!chunk.ok || chunk.at != size) {
        return out;
      }
    }
  }
  out.ok = r.ok && header;
  return out;
}

} // namespace Wwise::Bank
