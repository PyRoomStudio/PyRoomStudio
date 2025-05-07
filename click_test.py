from direct.showbase.ShowBase import ShowBase
from panda3d.core import (
    CardMaker,
    CollisionTraverser, CollisionNode, CollisionRay, CollisionHandlerQueue,
    BitMask32
)
import random

class ClickablePlaneDemo(ShowBase):
    def __init__(self):
        super().__init__()

        # 1) Create a 2×2 plane…
        cm = CardMaker('plane')
        cm.setFrame(-1, 1, -1, 1)
        self.plane = self.render.attachNewNode(cm.generate())

        # 2) Position it 5 units in front of the camera…
        self.plane.setPos(0, 5, 0)

        # 3) Rotate it *around the X‑axis* by 90° so its normal faces the camera.
        #    (roll=90°, not pitch!)
        self.plane.setHpr(0, 0, 90)

        # 4) Make sure both sides render (optional, but handy for one‑sided cards)
        self.plane.setTwoSided(True)

        # 5) Start it white and tag it for clicking
        self.plane.setColor(1, 1, 1, 1)
        self.plane.setTag('clickable', '1')
        self.plane.setCollideMask(BitMask32.bit(1))

        # 6) Set up the picking ray
        self.picker = CollisionTraverser()
        self.queue  = CollisionHandlerQueue()

        picker_node = CollisionNode('mouseRay')
        picker_np   = self.camera.attachNewNode(picker_node)
        picker_node.addSolid(CollisionRay())
        picker_node.setFromCollideMask(BitMask32.bit(1))
        picker_node.setIntoCollideMask(BitMask32.allOff())
        self.picker.addCollider(picker_np, self.queue)
        # stash a reference to the ray so we can update it each click
        self.ray = picker_node.getSolid(0)

        # 7) Hook up left‑mouse clicks
        self.accept('mouse1', self.on_click)

    def on_click(self):
        if not self.mouseWatcherNode.hasMouse():
            return

        # turn mouse X/Y into a 3D ray
        mx, my = self.mouseWatcherNode.getMouse()
        self.ray.setFromLens(self.camNode, mx, my)

        # traverse and see what we hit
        self.picker.traverse(self.render)

        if self.queue.getNumEntries() > 0:
            self.queue.sortEntries()
            entry = self.queue.getEntry(0)
            picked_np = entry.getIntoNodePath().getParent()
            if picked_np.getTag('clickable') == '1':
                # boom—new random color
                picked_np.setColor(
                    random.random(),
                    random.random(),
                    random.random(),
                    1
                )

app = ClickablePlaneDemo()
app.run()
