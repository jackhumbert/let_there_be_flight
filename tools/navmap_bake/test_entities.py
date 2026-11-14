"""Offline checks of entities.py on a street lamp placement (python -m unittest test_entities).

The sector below is cut down from exterior_-31_-17_0_0 (game 2.5): a highway lamp placed by a
worldEntityNode whose template is embedded in the sector, built like street_lamp_a.ent (a
destruction mesh whose intact pole is a Kinematic body, a World Static base, a "Bullet logic"
collider bound to the lamp's light), and a pipe lamp built like street_lamp_g_jt.ent (Soft
template, Static physical meshes with visualScale). The numbers are the real ones of those files;
the convex base is a box here.
"""

import json
import os
import sys
import unittest

import numpy as np

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))

import entities  # noqa: E402

FP = 131072
DESTRUCTIBLE = {"preset": "Destructible", "mask1": 16448, "mask2": 765}
DEBRIS = {"preset": "Debris", "mask1": 4096, "mask2": 236}
WORLD_STATIC = {"preset": "World Static", "mask1": 376836, "mask2": 5070}
BULLET = {"preset": "Bullet logic", "mask1": 0, "mask2": 0}
LAMP_AT = (-1984.98682, -1076.83899, 57.5802689)


def handle(hid, data):
    return {"HandleId": str(hid), "Data": data}


def fd(f):
    return {
        "$type": "physicsFilterData",
        "preset": {"$type": "CName", "$storage": "string", "$value": f["preset"]},
        "simulationFilter": {"mask1": str(f["mask1"]), "mask2": str(f["mask2"])},
    }


def name(n):
    return {"$type": "CName", "$storage": "string", "$value": n}


def res(path, flags="Default"):
    return {"DepotPath": {"$type": "ResourcePath", "$storage": "string", "$value": path}, "Flags": flags}


def wt(x=0.0, y=0.0, z=0.0, q=(0, 0, 0, 1)):
    return {
        "$type": "WorldTransform",
        "Position": {c: {"Bits": round(v * FP)} for c, v in zip("xyz", (x, y, z))},
        "Orientation": dict(zip("ijkr", q)),
    }


def tr(x=0.0, y=0.0, z=0.0, q=(0, 0, 0, 1)):
    return {"position": {"X": x, "Y": y, "Z": z, "W": 0}, "orientation": dict(zip("ijkr", q))}


def box(half, at=tr(), filt=None):
    s = {"$type": "physicsColliderBox", "halfExtents": dict(zip("XYZ", half)), "localToBody": at,
         "isQueryShapeOnly": 0, "filterData": filt}
    return s


DST_MESH = r"base\environment\decoration\lighting\street\street_lamp\destruction\street_lamp_a_dst.mesh"
BASE_MESH = r"base\environment\decoration\lighting\street\street_lamp\dstr\dstr_street_lamp_base_a.mesh"
PIPE_MESH = r"base\environment\architecture\common\electric_tower\electric_tower_b_pipe.mesh"
LAMP_TPL = r"base\gameplay\devices\lighting\street\street_lamps\street_lamp_a_highway_glen.ent"
PIPE_TPL = r"base\gameplay\devices\lighting\street\street_lamps\street_lamp_g_jt.ent"
ROT_Z180 = (0, 0, 1, 0)


def lamp_sector():
    """A sector with the embedded lamp template placed once, the pipe lamp (Soft) once, and a
    Dynamic destructible prop node."""
    components = [
        {"$type": "entPhysicalDestructionComponent", "name": name("desctructible_pole"),
         "localTransform": wt(z=-19661 / FP), "parentTransform": None, "mesh": res(DST_MESH),
         "destructionLevelData": [{"filterData": handle(1, fd(DESTRUCTIBLE))}, {"filterData": handle(2, fd(DEBRIS))}]},
        {"$type": "gameLightComponent", "name": name("L_Lamp"), "localTransform": wt(y=1.155, z=9.44),
         "parentTransform": handle(3, {"$type": "entHardTransformBinding", "bindName": name("desctructible_pole")})},
        {"$type": "entPhysicalMeshComponent", "name": name("dstr_street_lamp_base_b"), "localTransform": wt(),
         "parentTransform": None, "mesh": res(BASE_MESH), "simulationType": "Kinematic",
         "filterData": handle(4, fd(WORLD_STATIC)), "filterDataSource": "Collider"},
        {"$type": "entColliderComponent", "name": name("ColliderShared"), "localTransform": wt(0.04, 0.07, 1.31),
         "parentTransform": handle(5, {"$type": "entHardTransformBinding", "bindName": name("L_Lamp")}),
         "simulationType": "Kinematic", "filterData": handle(6, fd(BULLET)),
         "colliders": [handle(7, box((0.2, 0.4, 0.1)))]},
    ]
    template = {"$type": "entEntityTemplate", "components": components, "appearances": [],
                "defaultAppearance": name("default"), "includes": []}
    nodes = [
        {"Data": {"$type": "worldEntityNode", "entityTemplate": res(LAMP_TPL, "Embedded"), "appearanceName": name("default")}},
        {"Data": {"$type": "worldDeviceNode", "entityTemplate": res(PIPE_TPL, "Soft"), "appearanceName": name("default")}},
        {"Data": {"$type": "worldInstancedDestructibleMeshNode", "mesh": res(PIPE_MESH), "simulationType": "Dynamic",
                  "filterData": handle(8, fd(WORLD_STATIC)), "filterDataSource": "Parent", "instanceTransforms": [tr()]}},
    ]
    node_data = [
        {"nodeIdx": "0", "transform": tr(*LAMP_AT), "scale": {"X": 1, "Y": 1, "Z": 1}},
        {"nodeIdx": "1", "transform": tr(-2021.40955, -1085.23962, 8.08016968), "scale": {"X": 1, "Y": 1, "Z": 1}},
        {"nodeIdx": "2", "transform": tr(-2000.0, -1080.0, 8.0), "scale": {"X": 1, "Y": 1, "Z": 1}},
    ]
    return {"Data": {
        "RootChunk": {"nodes": nodes, "nodeData": {"Data": node_data}},
        "EmbeddedFiles": [{"FileName": {"$type": "ResourcePath", "$storage": "string", "$value": LAMP_TPL},
                           "Content": template}],
    }}


def pipe_template():
    components = [
        {"$type": "entPhysicalMeshComponent", "name": name("street_lamp_g8662"), "localTransform": wt(z=-1311 / FP),
         "parentTransform": None, "mesh": res(PIPE_MESH), "simulationType": "Static", "filterDataSource": "Collider",
         "filterData": handle(1, fd(WORLD_STATIC)), "visualScale": {"X": 1.2, "Y": 1.2, "Z": 1.2}},
        {"$type": "entPhysicalMeshComponent", "name": name("swinging_sign"), "localTransform": wt(z=3.0),
         "parentTransform": None, "mesh": res(PIPE_MESH), "simulationType": "Dynamic", "filterDataSource": "Collider",
         "filterData": None},
    ]
    doc = {"Data": {"RootChunk": {"$type": "entEntityTemplate", "components": components, "appearances": [],
                                  "defaultAppearance": name("default"), "includes": []}}}
    return entities.template_compact(doc["Data"]["RootChunk"], entities.handle_table(doc), {})


def mesh_doc(bodies):
    return {"Data": {"RootChunk": {"parameters": [handle(100, {
        "$type": "meshMeshParamPhysics",
        "physicsData": handle(101, {"$type": "physicsSystemResource", "bodies": [handle(102 + i, b) for i, b in enumerate(bodies)]}),
    })]}}}


def body(sim, at, shapes, query=0):
    return {"$type": "physicsSystemBody", "isQueryBodyOnly": query, "localToModel": at,
            "params": {"simulationType": sim}, "collisionShapes": [handle(200 + i, s) for i, s in enumerate(shapes)]}


def meshes():
    dst = mesh_doc([
        # the intact lamp: plexiglass head (Debris) and the pole (no filter: the component's level 0)
        body("Kinematic", tr(-0.000235596, 0.710294843, 5.0132184, ROT_Z180), [
            box((0.283985585, 0.918480515, 0.136277109),
                tr(0, -0.0941922665, 4.36980009, (-3.2640699e-07, 0.968147695, -0.250379831, 3.92650144e-08)),
                handle(300, fd(DEBRIS))),
            box((0.115824282, 0.0697093084, 4.32649899), tr(0, 0.703840673, -0.402746677, ROT_Z180)),
        ]),
        # a broken piece
        body("Dynamic", tr(0, 0, 3), [box((0.12, 0.07, 3.0), filt=handle(301, fd(DESTRUCTIBLE)))]),
    ])
    base = mesh_doc([body("Static", tr(), [box((0.5, 0.5, 0.25), tr(z=-0.15), handle(302, fd(WORLD_STATIC)))])])
    pipe = mesh_doc([body("Static", tr(), [box((0.15, 0.15, 6.5), tr(z=6.5), handle(303, fd(WORLD_STATIC)))])])
    return {DST_MESH: entities.mesh_compact(dst), BASE_MESH: entities.mesh_compact(base),
            PIPE_MESH: entities.mesh_compact(pipe)}


def build(min_size=0.6):
    doc = json.loads(json.dumps(lamp_sector()))  # as read from a file
    placements = json.loads(json.dumps(entities.sector_placements(doc)))
    store = {"ent": {PIPE_TPL: pipe_template()}, "app": {}}
    builder = entities.Builder(meshes(), min_size)
    entities.build_placements(placements, store, builder.meshes, builder)
    return placements, builder.arrays(), builder


class LampPlacement(unittest.TestCase):
    def test_placements_read(self):
        placements, _, _ = build()
        self.assertEqual([e[0] for e in placements["entities"]], [LAMP_TPL, PIPE_TPL])
        self.assertEqual([e[1] for e in placements["entities"]], [True, False])
        self.assertIn(LAMP_TPL, placements["embedded"])
        self.assertEqual(len(placements["meshes"]), 1)

    def by_source(self, arrays, path):
        i = list(arrays["source_names"]).index(path)
        return arrays["tris"][arrays["source"] == i], arrays["kind"][arrays["source"] == i]

    def test_highway_lamp_pole_is_solid(self):
        _, arrays, _ = build()
        tris, kind = self.by_source(arrays, LAMP_TPL)
        v = tris.reshape(-1, 3)
        x, y, z = LAMP_AT
        # the pole (destructible, intact body) stands from 0.13 m to 8.79 m above the node (the
        # component sits 0.15 m down), within a few decimetres of it sideways
        pole = tris[kind == entities.KIND_DESTRUCTIBLE].reshape(-1, 3)
        self.assertEqual(len(pole), 12 * 3)  # one box: the Debris head is left out
        self.assertAlmostEqual(pole[:, 2].min() - z, 0.13, delta=0.02)
        self.assertAlmostEqual(pole[:, 2].max() - z, 8.79, delta=0.02)
        self.assertLess(np.abs(pole[:, 0] - x).max(), 0.2)
        self.assertLess(np.abs(pole[:, 1] - y).max(), 0.2)
        # base box: Kinematic component, World Static shape
        base = tris[kind == entities.KIND_KINEMATIC].reshape(-1, 3)
        self.assertEqual(len(base), 12 * 3)
        self.assertAlmostEqual(base[:, 2].min() - z, -0.40, delta=0.01)
        # nothing else: no broken pieces, no Bullet logic collider
        self.assertEqual(len(v), 2 * 12 * 3)
        self.assertEqual(sorted(set(arrays["preset_names"][arrays["preset"][arrays["source"] == 0]])),
                         ["Destructible", "World Static"])
        self.assertTrue(arrays["blocks"].all())

    def test_pipe_lamp_scaled_and_dynamic_left_out(self):
        _, arrays, builder = build()
        tris, kind = self.by_source(arrays, PIPE_TPL)
        v = tris.reshape(-1, 3)
        self.assertEqual(len(v), 12 * 3)  # the Dynamic sign is left out
        self.assertTrue((kind == entities.KIND_STATIC).all())
        # 13 m pipe at visualScale 1.2: 15.6 m, from 1311/131072 m below the node
        self.assertAlmostEqual(v[:, 2].min() - 8.08016968, -0.012, delta=0.01)
        self.assertAlmostEqual(v[:, 2].max() - v[:, 2].min(), 15.6, delta=0.01)
        self.assertNotIn(PIPE_MESH, list(arrays["source_names"]))  # the Dynamic prop node
        self.assertGreaterEqual(builder.stats["dynamic parts"], 2)

    def test_small_placements_dropped(self):
        _, arrays, builder = build(min_size=20.0)
        self.assertIsNone(arrays)
        self.assertEqual(builder.stats["small placements"], 2)

    def test_touches_car(self):
        f = lambda d: [d["preset"], d["mask1"], d["mask2"]]  # noqa: E731
        self.assertTrue(entities.touches_car(f(WORLD_STATIC)))
        self.assertTrue(entities.touches_car(f(DESTRUCTIBLE)))
        self.assertFalse(entities.touches_car(f(DEBRIS)))
        self.assertFalse(entities.touches_car(f(BULLET)))
        self.assertFalse(entities.touches_car(None))


if __name__ == "__main__":
    unittest.main()
