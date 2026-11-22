// The landing pad list the plugin ships (src/navigation/helipads.tsv, src/red4ext/Navigation/Helipads): read
// as the plugin reads it, every pad numbered once (tools/pads/pad_numbers.py; the numbers are painted on the decks).
#include "check.hpp"

#include "Navigation/Helipads.hpp"

#include <filesystem>
#include <fstream>
#include <set>

using namespace Navigation;

TEST(helipads_shipped_list_numbered_once) {
  auto &pads = Helipads::Get();
  CHECK(pads.Load(LTBF_SOURCE_DIR "/src/navigation/helipads.tsv"));
  CHECK(pads.All().size() > 50);
  std::set<int32_t> seen;
  for (const auto &pad : pads.All()) {
    CHECK(pad.number > 0);
    CHECK(seen.insert(pad.number).second);
  }
}

TEST(helipads_line_name_and_number_optional) {
  const auto path = std::filesystem::temp_directory_path() / "ltbf_helipads_test.tsv";
  {
    std::ofstream out(path);
    out << "# id\tx\ty\tz\tyaw\tradius\tclearance\tname\tnumber\n";
    out << "a\t1\t2\t3\t90\t6\tinf\tNamed\t14\n";
    out << "b\t1\t2\t3\t90\t6\t12.5\t\t7\n";
    out << "c\t1\t2\t3\t90\t6\tinf\t\n"; // an older list: no number column
  }
  auto &pads = Helipads::Get();
  CHECK(pads.Load(path));
  CHECK(pads.All().size() == 3);
  CHECK(pads.All()[0].name == "Named" && pads.All()[0].number == 14);
  CHECK(pads.All()[1].name.empty() && pads.All()[1].number == 7);
  CHECK(pads.All()[2].number == 0);
  std::filesystem::remove(path);
  pads.Load(LTBF_SOURCE_DIR "/src/navigation/helipads.tsv"); // leave the shipped list for other tests
}
