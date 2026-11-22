"""Offline checks of pad_numbers.py (python -m unittest test_pad_numbers, from tools/pads): numbers are kept and
never reused, new pads follow map reading order, digits sit on the sign's +X side first, a sign goes on the vanilla
28 (hidden by its nodeData index) or turned 90 degrees on a plain border, and the committed files agree.
"""

import json
import os
import sys
import unittest

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))

import pad_numbers  # noqa: E402


def row(pad_id, x, y, z, number=""):
    return [pad_id, str(x), str(y), str(z), "0.0", "6.0", "inf", "", str(number)]


class Numbers(unittest.TestCase):
    def test_kept_numbers_stay_and_new_ones_follow_the_highest(self):
        # 3 was given to a pad no longer listed: it stays retired
        out = pad_numbers.assign({"a": (0, 0), "b": (100, 0)}, {"a": 1, "gone": 3})
        self.assertEqual(out, {"a": 1, "gone": 3, "b": 4})

    def test_reading_order_north_band_first_then_west_to_east(self):
        pads = {"south_west": (-900, -1200), "north_east": (800, 900), "north_west": (-700, 950)}
        out = pad_numbers.assign(pads, {})
        self.assertEqual(out, {"north_west": 1, "north_east": 2, "south_west": 3})

    def test_a_number_given_twice_is_refused(self):
        with self.assertRaises(SystemExit):
            pad_numbers.assign({}, {"a": 1, "b": 1})

    def test_digits_first_on_plus_x_a_whole_number_of_diode_pitches_apart(self):
        self.assertEqual(pad_numbers.digit_offsets(7), [(7, 0.0)])
        (d0, x0), (d1, x1) = pad_numbers.digit_offsets(28)
        self.assertEqual((d0, d1), (2, 8))
        self.assertGreater(x0, 0.0)
        self.assertAlmostEqual(x0, -x1)
        three = pad_numbers.digit_offsets(105)
        self.assertEqual([d for d, _ in three], [1, 0, 5])
        self.assertAlmostEqual(three[1][1], 0.0)
        for n in (28, 105):
            xs = [x for _, x in pad_numbers.digit_offsets(n)]
            step = (xs[0] - xs[1]) / pad_numbers.PITCH
            self.assertAlmostEqual(step, round(step))
        # glyphs keep a gap of at least 10 px (the vanilla pair has 15), at their drawn size
        px = 5.0 / 128
        for n in (28, 105):
            xs = [x for _, x in pad_numbers.digit_offsets(n)]
            width = pad_numbers.GLYPH_W * pad_numbers.DIGIT_SCALE[len(str(n))]
            self.assertGreaterEqual((xs[0] - xs[1] - width) / px, 10.0, n)
        # three digits stay inside the ring (inner edge 53.5 px from the centre), corners included (glyph 57 px tall)
        s = pad_numbers.DIGIT_SCALE[3]
        corner = (three[0][1] / px + 11 * s, 28.5 * s)
        self.assertLess((corner[0] ** 2 + corner[1] ** 2) ** 0.5, 50.0)


class Signs(unittest.TestCase):
    PLACEMENTS = [
        {"mesh": "landing_pad_a_01", "pos": [10.0, 20.0, 50.0], "yaw": 30.0, "id": "exterior_0_0_0_0/4"},
        {"mesh": "landing_pad_b_01", "pos": [10.0, 20.0, 50.0], "yaw": 120.0, "id": "exterior_0_0_0_0/5"},
        {"mesh": "landing_pad_a_01", "pos": [300.0, 20.0, 9.6], "yaw": -35.0, "id": "exterior_1_0_0_1/167/264"},
    ]

    def test_sign_on_the_28_hides_it_by_index_and_a_border_alone_gets_one_turned(self):
        rows = [row("with28", 10.3, 20.1, 50.5, 7), row("border", 300.0, 20.0, 13.0, 8), row("prefab", 900, 0, 5, 9)]
        signs, bare = pad_numbers.find_signs(rows, self.PLACEMENTS)
        by = {s["pad"]: s for s in signs}
        self.assertEqual(bare, ["prefab"])
        self.assertEqual(by["with28"]["yaw"], 120.0)
        self.assertEqual(by["with28"]["hide"], {"sector": "exterior_0_0_0_0", "index": 5, "x": 10.0, "y": 20.0})
        self.assertEqual(by["border"]["yaw"], 55.0)
        self.assertIsNone(by["border"]["hide"])
        self.assertEqual(by["border"]["pos"][2], 9.6)

    def test_plain_small_prefab_deck_gets_a_sign_in_its_middle(self):
        prefab = {"mesh": "av_landing_pad_a_v3_small", "pos": [100.0, 200.0, 5.85], "yaw": 90.0, "id": "4242"}
        signs, bare = pad_numbers.find_signs([row("4242", 100.0, 202.0, 7.5, 45)], [prefab])
        self.assertEqual(bare, [])
        ox, oy = pad_numbers.PAINTED_PREFABS["av_landing_pad_a_v3_small"]
        # turned 90 degrees: the prefab's -X end offset lands along -Y
        self.assertAlmostEqual(signs[0]["pos"][0], 100.0 - oy, places=3)
        self.assertAlmostEqual(signs[0]["pos"][1], 200.0 + ox, places=3)
        self.assertEqual(signs[0]["pos"][2], 7.5)
        self.assertIsNone(signs[0]["hide"])


class Committed(unittest.TestCase):
    def test_tsv_numbers_signs_and_hidden_nodes_agree(self):
        numbers = pad_numbers.load()
        _, rows = pad_numbers.read_tsv(pad_numbers.TSV)
        for r in rows:
            self.assertEqual(int(r[8]), numbers[r[0]], r[0])
        self.assertEqual(len(set(numbers.values())), len(numbers))
        signs = json.load(open(pad_numbers.SIGNS, encoding="utf-8"))["signs"]
        listed = {r[0]: int(r[8]) for r in rows}
        for s in signs:
            self.assertEqual(listed.get(s["pad"]), s["number"], s["pad"])
        hidden = open(pad_numbers.HIDDEN, encoding="utf-8").read()
        block = hidden[hidden.index(pad_numbers.HIDDEN_BEGIN):hidden.index(pad_numbers.HIDDEN_END)]
        lines = [line for line in block.splitlines() if line.startswith("I ")]
        self.assertEqual(len(lines), sum(1 for s in signs if s["hide"]))


if __name__ == "__main__":
    unittest.main()
