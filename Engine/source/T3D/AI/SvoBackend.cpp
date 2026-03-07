#include "T3D/AI/SvoBackend.h"
#include "scene/sceneContainer.h"
#include "gfx/gfxDrawUtil.h"
#include "gfx/gfxTransformSaver.h"
#include <queue>
#include "navigation/navPath.h"
#include "math/mathIO.h"
#include "core/util/safeDelete.h"
#include "core/util/tDictionary.h" // For Map

static U32 sAILoSMask = TerrainObjectType | StaticObjectType | StaticShapeObjectType;

IMPLEMENT_CO_NETOBJECT_V1(NavSvo);

NavSvo::NavSvo()
   : mRoot(NULL),
   mSeedPos(Point3F::Zero),
   mFidelity(1.0f),
   mfull3d(true),
   mBuildNav(false),
   mSaveNav(false),
   mRender(false)
{
   mDataFileName = StringTable->EmptyString();
}

NavSvo::~NavSvo()
{
   clear();
}

bool NavSvo::onAdd()
{
   if (!Parent::onAdd())
      return false;

   if (isServerObject())
   {
      if (loadFromFile())
         mHasDirtyData = true;
   }
   return true;
}

void NavSvo::onRemove()
{
   clear();
   Parent::onRemove();
}

void NavSvo::initPersistFields()
{
    SceneObject::initPersistFields();    addField("seedPos", TypePoint3F, Offset(mSeedPos, NavSvo), "Initial seed position for SVO floodfill.");
    addField("fidelity", TypeF32, Offset(mFidelity, NavSvo), "Step size for SVO floodfill (voxel size).");
    addField("full3d", TypeBool, Offset(mfull3d, NavSvo), "Whether to perform full 3D floodfill or just 2D (X/Y axis).");
    addField("dataFile", TypeFilename, Offset(mDataFileName, NavSvo), "The filename for saving/loading the SVO navmesh.");
    addProtectedField("buildNav", TypeBool, Offset(mBuildNav, NavSvo),
       &_buildNav, &defaultProtectedGetFn, "build", AbstractClassRep::FieldFlags::FIELD_ComponentInspectors);
    addProtectedField("saveNav", TypeBool, Offset(mSaveNav, NavSvo),
          &_saveNav, &defaultProtectedGetFn, "save", AbstractClassRep::FieldFlags::FIELD_ComponentInspectors);
    addField("render", TypeBool, Offset(mRender, NavSvo), "Render the SVO for debugging.");

}

void NavSvo::inspectPostApply()
{
   Parent::inspectPostApply();
   mHasDirtyData = true;
}

bool NavSvo::_buildNav(void* object, const char* index, const char* data)
{
   NavSvo* svo = reinterpret_cast<NavSvo*>(object);
   svo->buildSVO();
   svo->setMaskBits(-1);
   return false;
}

bool NavSvo::_saveNav(void* object, const char* index, const char* data)
{
   NavSvo* svo = reinterpret_cast<NavSvo*>(object);
   svo->saveToFile();
   svo->setMaskBits(-1);
   return false;
}

void NavSvo::rebuildSpatialIndex()
{
    mSpatialIndex.clear();
    if (!mRoot) return;
    mRoot->forEachLeafAs<NavNode>([&](NavNode* node) {
        mSpatialIndex.insert(node->bounds().getCenter(), node);
    });
}

void NavSvo::clear()
{
    if (mRoot)
    {
        // Clear all neighbor vectors in the tree before deleting nodes
        mRoot->forEachInSubtree([](TreeNode<NavData>* node) {
            static_cast<NavNode*>(node)->data.neighbors.clear();
        });

        mRoot->nullParent();
        mRoot->deleteChildren();
        delete mRoot;
        mRoot = NULL;
    }
    mSpatialIndex.clear();
}


// Utility: Clustered raycast (center + 8 offsets), returns true if any hit is valid for the ray direction
bool NavSvo::clusterRaycast(const Point3F& from, const Point3F& to, U32 mask, F32 maxSlopeCos, F32 rayLen, bool isCeiling)
{
   static const S32 numOffsets = 8;
   static const Point2F circleOffsets[numOffsets] = {
      Point2F( 1,  1), Point2F(-1,  1), Point2F( 1, -1), Point2F(-1, -1),
      Point2F( 1,  0), Point2F(-1,  0), Point2F( 0,  1), Point2F( 0, -1)
   };

   Point3F delta = to - from;
   Point3F dir = delta;
   dir.normalizeSafe();

   // Find two perpendicular vectors to dir
   Point3F u, v;
   // Pick a vector not parallel to dir
   if (mFabs(dir.z) < 0.99f)
      u = mCross(dir, Point3F(0,0,1));
   else
      u = mCross(dir, Point3F(0,1,0));
   u.normalizeSafe();
   v = mCross(dir, u);
   v.normalizeSafe();

   F32 offsetRadius = mFidelity * 0.25f;

   // Center
   RayInfo ri;
   if (gServerContainer.castRay(from, to, mask, &ri)) {
      if (mFabs(dir.z) > 0.99f) { // vertical
         if (!isCeiling && ri.normal.z >= maxSlopeCos && ri.normal.z > 0 && ri.distance >= 0 - POINT_EPSILON && ri.distance <= rayLen + POINT_EPSILON)
            return true;
         if (isCeiling && ri.normal.z < 0 && ri.distance >= 0 - POINT_EPSILON && ri.distance <= rayLen + POINT_EPSILON)
            return true;
      } else if (mFabs(dir.z) < 0.01f) { // horizontal
         return true;
      } else {
         return true;
      }
   }

   // 8 offsets in the plane perpendicular to the ray
   for (S32 i = 0; i < numOffsets; ++i) {
      Point3F offset = u * (circleOffsets[i].x * offsetRadius) + v * (circleOffsets[i].y * offsetRadius);
      Point3F oFrom = from + offset;
      Point3F oTo = to + offset;
      RayInfo extraRi;
      if (gServerContainer.castRay(oFrom, oTo, mask, &extraRi)) {
         if (mFabs(dir.z) > 0.99f) {
            if (!isCeiling && extraRi.normal.z >= maxSlopeCos && extraRi.normal.z > 0 && extraRi.distance >= 0 - POINT_EPSILON && extraRi.distance <= rayLen + POINT_EPSILON)
               return true;
            if (isCeiling && extraRi.normal.z < 0 && extraRi.distance >= 0 - POINT_EPSILON && extraRi.distance <= rayLen + POINT_EPSILON)
               return true;
         } else if (mFabs(dir.z) < 0.01f) {
            return true;
         } else {
            return true;
         }
      }
   }
   return false;
}

bool NavSvo::isLeafNavigable(const Box3F& box)
{
   // always mark the leaf containing the seed position as navigable
   if (box.isContained(getWorldSeedPos()))
      return true;

   F32 rayLen = box.len_z();
   Point3F center = box.getCenter();
   Point3F top(center.x, center.y, box.maxExtents.z);
   Point3F bottom(center.x, center.y, box.minExtents.z);
   Point3F north(center.x, box.maxExtents.y, center.z);
   Point3F south(center.x, box.minExtents.y, center.z);
   Point3F east(box.maxExtents.x, center.y, center.z);
   Point3F west(box.minExtents.x, center.y, center.z);

   const F32 maxSlopeCos = mCos(mDegToRad(45.0f));

   // Floor check (walkable if normal points up and within slope)
   bool foundWalkable = clusterRaycast(top, bottom, sAILoSMask, maxSlopeCos, rayLen, false);

   // Wall checks (block if any wall cluster returns true)
   bool wallBlocked = false;
   wallBlocked |= clusterRaycast(west, east, sAILoSMask, maxSlopeCos, rayLen, false);
   wallBlocked |= clusterRaycast(east,  west, sAILoSMask, maxSlopeCos, rayLen, false);
   wallBlocked |= clusterRaycast(south, north, sAILoSMask, maxSlopeCos, rayLen, false);
   wallBlocked |= clusterRaycast(north, south, sAILoSMask, maxSlopeCos, rayLen, false);

   // Ceiling check (blocked if normal points down)
   bool ceilingBlocked = clusterRaycast(bottom, top, sAILoSMask, maxSlopeCos, rayLen, true);

   bool navigable = false;
   if (!wallBlocked && !ceilingBlocked) {
      if (foundWalkable) {
         navigable = true;
      } else if (mfull3d) {
         // In-air check: no vertical obstruction (center and offsets)
         bool inAir = !clusterRaycast(bottom, top, sAILoSMask, maxSlopeCos, rayLen, false);
         navigable = inAir;
      }
   }

   Con::printf("Leaf bounds: min(%f %f %f) max(%f %f %f) navigable: %s",
      box.minExtents.x, box.minExtents.y, box.minExtents.z,
      box.maxExtents.x, box.maxExtents.y, box.maxExtents.z,
      navigable ? "true" : "false");

   return navigable;
}

void NavSvo::pruneOrphans(NavNode* start)
{
    // Flood fill: collect all reachable nodes
    Vector<NavNode*> open, visited;
    open.push_back(start);
    visited.push_back(start);
    for (U32 idx = 0; idx < open.size(); ++idx)
        for (NavNode* neighbor : open[idx]->data.neighbors)
            if (!visited.contains(neighbor)) {
                open.push_back(neighbor);
                visited.push_back(neighbor);
            }

    // Collect highest unreachable ancestors for each unreachable leaf
    Vector<NavNode*> toDelete;
    Point3F seedPos = getWorldSeedPos();
    mRoot->forEachLeafAs<NavNode>([&](NavNode* node) {
        if (node->bounds().isContained(seedPos) || visited.contains(node)) return;
        NavNode* ancestor = node;
        while (ancestor->getParent() && !visited.contains(static_cast<NavNode*>(ancestor->getParent())))
            ancestor = static_cast<NavNode*>(ancestor->getParent());
        bool allUnreachable = true;
        for (U32 i = 0; i < ancestor->getNumChildren(); ++i)
            if (visited.contains(ancestor->getChild(i))) { allUnreachable = false; break; }
        if (allUnreachable && !toDelete.contains(ancestor)) {
            toDelete.push_back(ancestor);
            const Box3F& box = ancestor->bounds();
            Con::printf("Pruning unreachable node: min(%f %f %f) max(%f %f %f)",
                box.minExtents.x, box.minExtents.y, box.minExtents.z,
                box.maxExtents.x, box.maxExtents.y, box.maxExtents.z);
        }
    });

    // Remove and delete unreachable subtrees
    deleteNodes(toDelete);
}

void NavSvo::pruneAllNeighbors()
{
   if (!mRoot) return;
   mRoot->forEachInSubtree([](TreeNode<NavData>* node) {
      NavNode* navNode = static_cast<NavNode*>(node);
      navNode->pruneNeighbors();
      });
}

void NavNode::pruneNeighbors()
{
   for (S32 i = data.neighbors.size() - 1; i >= 0; --i)
   {
      NavNode* neighbor = data.neighbors[i];
      if (!neighbor || !neighbor->data.navigable)
      {
         // Remove this node from the neighbor's neighbor list as well
         if (neighbor)
            neighbor->data.neighbors.remove(this);
         data.neighbors.erase(i);
      }
   }
}

Point3F NavSvo::getWorldSeedPos() const
{
   Point3F worldSeed = getPosition() + mSeedPos;
   F32 fx = mFidelity > 0.0f ? mFidelity : 1.0f;
   Box3F bounds = getWorldBox();
   return Point3F(
      mClampF(mFloor(worldSeed.x / fx) * fx + fx * 0.5f, bounds.minExtents.x, bounds.maxExtents.x - POINT_EPSILON),
      mClampF(mFloor(worldSeed.y / fx) * fx + fx * 0.5f, bounds.minExtents.y, bounds.maxExtents.y - POINT_EPSILON),
      mClampF(mFloor(worldSeed.z / fx) * fx + fx * 0.5f, bounds.minExtents.z, bounds.maxExtents.z - POINT_EPSILON)
   );
}

void NavSvo::buildSVO()
{
    // Validate fidelity
    Point3F extents = getWorldBox().getExtents();
    if (mFidelity < 0.001f || mFidelity > extents.x || mFidelity > extents.y || mFidelity > extents.z) {
        Con::errorf("NavSvo::buildSVO - Invalid fidelity: %f for world extents (%f, %f, %f)", mFidelity, extents.x, extents.y, extents.z);
        return;
    }
    // Clamp world box to grid using mClampF, mFloor, mCeil
    Box3F world = getWorldBox();
    world.minExtents.x = mFloor(world.minExtents.x / mFidelity) * mFidelity;
    world.minExtents.y = mFloor(world.minExtents.y / mFidelity) * mFidelity;
    world.minExtents.z = mFloor(world.minExtents.z / mFidelity) * mFidelity;
    world.maxExtents.x = mCeil(world.maxExtents.x / mFidelity) * mFidelity;
    world.maxExtents.y = mCeil(world.maxExtents.y / mFidelity) * mFidelity;
    world.maxExtents.z = mCeil(world.maxExtents.z / mFidelity) * mFidelity;

    clear();
    mRoot = new NavNode();
    mRoot->setBounds(world);

    Con::printf("SVO root bounds: min(%f %f %f) max(%f %f %f)",
       mRoot->bounds().minExtents.x, mRoot->bounds().minExtents.y, mRoot->bounds().minExtents.z,
       mRoot->bounds().maxExtents.x, mRoot->bounds().maxExtents.y, mRoot->bounds().maxExtents.z);

    Point3F worldSeed = getWorldSeedPos();

    // Clamp seed to root bounds to guarantee containment
    worldSeed.x = mClampF(worldSeed.x, world.minExtents.x, world.maxExtents.x - POINT_EPSILON);
    worldSeed.y = mClampF(worldSeed.y, world.minExtents.y, world.maxExtents.y - POINT_EPSILON);
    worldSeed.z = mClampF(worldSeed.z, world.minExtents.z, world.maxExtents.z - POINT_EPSILON);

    Con::printf("World seed position: %f %f %f", worldSeed.x, worldSeed.y, worldSeed.z);
    buildSVORecursive(mRoot, 0);

    if (!mRoot) return;

    debugLeafCount("after buildSVORecursive");

    rebuildSpatialIndex();
    assignNeighbors();

    NavNode* start = getNodeContaining(worldSeed, false);
    if (!start) {
        Con::printf("[SVO DEBUG] No leaf contains the seed position for flood fill! SVO tree may be degenerate.");
        return;
    }

    pruneOrphans(start);
    debugLeafCount("after pruning");
    debugSeedNode();

    rebuildSpatialIndex();
    assignNeighbors();

}

Vector<NavNode*> NavSvo::splitNode(NavNode* parent)
{
    Vector<NavNode*> children;
    children.reserve(8);

    const Box3F& box = parent->bounds();
    Point3F size = box.getExtents();
    Point3F min = box.minExtents;
    Point3F max = box.maxExtents;
    Point3F mid = (min + max) * 0.5f;

    // Snap and clamp min/max to parent bounds (no world param)
    min.x = mClampF(mFloor(min.x / mFidelity) * mFidelity, min.x, max.x);
    max.x = mClampF(mCeil(max.x / mFidelity) * mFidelity, min.x, max.x);
    min.y = mClampF(mFloor(min.y / mFidelity) * mFidelity, min.y, max.y);
    max.y = mClampF(mCeil(max.y / mFidelity) * mFidelity, min.y, max.y);
    min.z = mClampF(mFloor(min.z / mFidelity) * mFidelity, min.z, max.z);
    max.z = mClampF(mCeil(max.z / mFidelity) * mFidelity, min.z, max.z);

    Vector<Box3F> createdBoxes;

    for (int i = 0; i < 8; ++i)
    {
        Point3F cmin(
            (i & 1) ? mid.x : min.x,
            (i & 2) ? mid.y : min.y,
            (i & 4) ? mid.z : min.z
        );
        Point3F cmax(
            (i & 1) ? max.x : mid.x,
            (i & 2) ? max.y : mid.y,
            (i & 4) ? max.z : mid.z
        );

        Box3F childBox(cmin, cmax);

        Point3F childSize = childBox.getExtents();
        Con::printf("Child size: %f %f %f", childSize.x, childSize.y, childSize.z);

        if (childSize.x < mFidelity - POINT_EPSILON ||
            childSize.y < mFidelity - POINT_EPSILON ||
            childSize.z < mFidelity - POINT_EPSILON)
            continue;

        NavNode* child = new NavNode();
        child->setBounds(childBox);
        children.push_back(child);

        Con::printf("Created child box: min(%f %f %f) max(%f %f %f)", cmin.x, cmin.y, cmin.z, cmax.x, cmax.y, cmax.z);
    }

    // Instead of assigning children here, just return them
    return children;
}

void NavSvo::buildSVORecursive(NavNode* node, U32 depth)
{
    static const int MAX_DEPTH = 64;
    AssertFatal(depth < MAX_DEPTH, avar("NavSvo::buildSVORecursive - Aborting: tree depth exceeds %d", MAX_DEPTH));

    const Box3F& box = node->bounds();
    Point3F size = box.getExtents();

    // If this box is exactly mFidelity in all axes, make it a leaf
    if (mFabs(size.x - mFidelity) < POINT_EPSILON &&
        mFabs(size.y - mFidelity) < POINT_EPSILON &&
        mFabs(size.z - mFidelity) < POINT_EPSILON)
    {
        if (!isLeafNavigable(box))
        {
            node->nullParent();
            node->deleteChildren();
            Vector<NavNode*> nodesToDelete;
            nodesToDelete.push_back(node);
            deleteNodes(nodesToDelete);
            return;
        }
        node->data.navigable = true;
        return;
    }

    // Compute split points for each axis (grid-aligned)
    Point3F min = box.minExtents;
    Point3F max = box.maxExtents;

    F32 xSplit = (size.x > mFidelity + POINT_EPSILON) ? (mCeil((min.x + max.x) * 0.5f / mFidelity) * mFidelity) : max.x;
    F32 ySplit = (size.y > mFidelity + POINT_EPSILON) ? (mCeil((min.y + max.y) * 0.5f / mFidelity) * mFidelity) : max.y;
    F32 zSplit = (size.z > mFidelity + POINT_EPSILON) ? (mCeil((min.z + max.z) * 0.5f / mFidelity) * mFidelity) : max.z;

    // Generate up to 8 children, but only if their size is >= mFidelity
    Vector<NavNode*> children;
    for (int xi = 0; xi < 2; ++xi)
    for (int yi = 0; yi < 2; ++yi)
    for (int zi = 0; zi < 2; ++zi)
    {
        F32 x0 = (xi == 0) ? min.x : xSplit;
        F32 x1 = (xi == 0) ? xSplit : max.x;
        F32 y0 = (yi == 0) ? min.y : ySplit;
        F32 y1 = (yi == 0) ? ySplit : max.y;
        F32 z0 = (zi == 0) ? min.z : zSplit;
        F32 z1 = (zi == 0) ? zSplit : max.z;

        if (x1 - x0 < mFidelity - POINT_EPSILON ||
            y1 - y0 < mFidelity - POINT_EPSILON ||
            z1 - z0 < mFidelity - POINT_EPSILON)
            continue;

        Box3F childBox(Point3F(x0, y0, z0), Point3F(x1, y1, z1));
        NavNode* child = new NavNode();
        child->setBounds(childBox);
        children.push_back(child);
    }

    Con::printf("Node at depth %d split into %d children", depth, children.size());

    // Recursively build children, pruning non-navigable ones
    Vector<TreeNode<NavData>*> validChildren;
    for (U32 i = 0; i < children.size(); ++i)
    {
        NavNode* child = children[i];
        buildSVORecursive(child, depth + 1);
        if (child && (child->getNumChildren() > 0 || child->data.navigable))
            validChildren.push_back(child);
        else if (child)
        {
            child->nullParent();
            child->deleteChildren();
            Vector<NavNode*> nodesToDelete;
            nodesToDelete.push_back(child);
            deleteNodes(nodesToDelete);
        }
    }

    // Filter out any deleted/invalid children before adding
    Vector<TreeNode<NavData>*> filteredChildren;
    for (U32 i = 0; i < validChildren.size(); ++i)
    {
       TreeNode<NavData>* c = validChildren[i];
       // Defensive: skip if pointer is NULL or looks like deleted memory
       if (!c)
          continue;
       if (c->parent == NULL || c->parent == node) // Only allow if not deleted
          filteredChildren.push_back(c);
    }

    if (filteredChildren.empty())
    {
       node->nullParent();
       node->deleteChildren();
       Vector<NavNode*> nodesToDelete;
       nodesToDelete.push_back(node);
       deleteNodes(nodesToDelete);
       return;
    }

    // Assign valid children to parent
    node->addChildren(&filteredChildren);
}

void NavSvo::assignNeighbors()
{
    if (!mRoot)
        return;

    Vector<NavNode*> leaves;
    mRoot->forEachLeafAs<NavNode>([&](NavNode* node) {
        leaves.push_back(node);
    });

    static const Point3F offsets3D[] = {
        Point3F( mFidelity,  0,        0),
        Point3F(-mFidelity,  0,        0),
        Point3F( 0,          mFidelity,0),
        Point3F( 0,         -mFidelity,0),
        Point3F( mFidelity,  mFidelity,0),
        Point3F(-mFidelity,  mFidelity,0),
        Point3F( mFidelity, -mFidelity,0),
        Point3F(-mFidelity, -mFidelity,0),
        Point3F( 0,          0,        mFidelity)
    };
    static const Point3F offsets2D[] = {
        Point3F( mFidelity,  0,        0),
        Point3F(-mFidelity,  0,        0),
        Point3F( 0,          mFidelity,0),
        Point3F( 0,         -mFidelity,0)
    };

    const Point3F* offsets = mfull3d ? offsets3D : offsets2D;
    U32 numOffsets = mfull3d ? (sizeof(offsets3D)/sizeof(Point3F)) : (sizeof(offsets2D)/sizeof(Point3F));

    for (U32 n = 0; n < leaves.size(); ++n)
    {
        NavNode* node = leaves[n];
        if (!node->data.navigable)
            continue;
        Point3F center = node->bounds().getCenter();
        for (U32 i = 0; i < numOffsets; ++i)
        {
            Point3F neighborCenter = center + offsets[i];
            NavNode* neighbor = NULL;
            if (mSpatialIndex.tryGetValue(neighborCenter, neighbor) && neighbor && node < neighbor)
            {
                if (!neighbor->data.navigable)
                    continue;
                node->setNeighbor(neighbor, true);
                neighbor->setNeighbor(node, true);
            }
        }
    }
}

// Specialized bulk deletion for NavNode, with neighbor cleanup
void NavSvo::deleteNodes(Vector<NavNode*>& nodesToDelete)
{
    // 1. Mark phase
    for (NavNode* node : nodesToDelete)
    {
        if (node == mRoot)
            continue; // Never mark root for deletion
        node->pendingDelete = true;
    }

    // 2. Sweep phase
    bool deletedAny;
    do {
        deletedAny = false;
        Vector<NavNode*> stillPending;
        for (NavNode* node : nodesToDelete) {
            if (node == mRoot || !node->pendingDelete)
                continue;
            bool hasPendingChild = false;
            for (U32 i = 0; i < node->getNumChildren(); ++i) {
                NavNode* child = node->getChild(i);
                if (child && child->pendingDelete) {
                    hasPendingChild = true;
                    break;
                }
            }
            if (!hasPendingChild) {
                // Cleanup neighbor references before deletion
                NavNode* root = static_cast<NavNode*>(node->getRoot());
                removeNeighborReferences(root, node);

                // Null out all references to this node in its parent
                if (node->getParent()) {
                    TreeNode<NavData>* parent = node->getParent();
                    for (U32 i = 0; i < parent->getNumChildren(); ++i) {
                        if (parent->getChild(i) == node) {
                            parent->nullChild(i);
                        }
                    }
                }

                // Nullify all child->parent pointers
                for (U32 i = 0; i < node->getNumChildren(); ++i) {
                    NavNode* child = node->getChild(i);
                    if (child && child->getParent() == node) {
                        child->nullParent();
                    }
                }

                node->nullParent();
                node->clearChildren();

                node->pendingDelete = false;
                SAFE_DELETE(node);
                deletedAny = true;
            } else {
                stillPending.push_back(node);
            }
        }
        nodesToDelete = stillPending;
    } while (deletedAny && !nodesToDelete.empty());
}

void NavSvo::removeNeighborReferences(NavNode* root, NavNode* node)
{
   if (!root || !node) return;
   root->data.neighbors.remove(node);
   for (U32 i = 0; i < root->getNumChildren(); ++i)
      removeNeighborReferences(root->getChild(i), node);
}

NavNode* NavSvo::findNearestLeaf(const Point3F& pt) const
{
    if (!mRoot)
        return NULL;

    NavNode* best = NULL;
    F32 minDist = F32_MAX;

    mRoot->forEachLeafAs<NavNode>([&](NavNode* leaf) {
        F32 dist = (leaf->bounds().getCenter() - pt).lenSquared();
        if (dist < minDist) {
            minDist = dist;
            best = leaf;
        }
    });

    return best;
}

void NavSvo::pointToNodeCoord(Point3F* pt)
{
   if (!pt)
      return;
   F32 fx = mFidelity > 0.0f ? mFidelity : 1.0f;
   pt->x = mFloor(pt->x / fx) * fx + fx * 0.5f;
   pt->y = mFloor(pt->y / fx) * fx + fx * 0.5f;
   pt->z = mFloor(pt->z / fx) * fx + fx * 0.5f;
}

NavNode* NavSvo::getNodeContaining(const Point3F& pos, bool testOccupied) const
{
   NavNode* node = mRoot;
   while (node && node->getNumChildren() > 0)
   {
      bool found = false;
      for (U32 i = 0; i < node->getNumChildren(); ++i)
      {
         NavNode* child = node->getChild(i);
         if (child && child->bounds().isContained(pos))
         {
            node = child;
            found = true;
            break;
         }
      }
      if (!found)
         return NULL; // No child contains the point
   }
   // Only return if not occupied (if requested), and contains the point
   if (node && node->bounds().isContained(pos) && (!testOccupied || !node->data.occupied))
      return node;
   return NULL;
}

NavNode* NavSvo::getNearestNode(const Point3F& pos, bool testOccupied) const
{
    // Fast path: if the point is inside a node, return it immediately
    NavNode* containing = getNodeContaining(pos, testOccupied);
    if (containing)
        return containing;

    if (!mRoot)
        return NULL;

    NavNode* best = NULL;
    F32 minDist = F32_MAX;

    mRoot->forEachLeafAs<NavNode>([&](NavNode* leaf) {
        if (!testOccupied || !leaf->data.occupied) {
            F32 dist = (leaf->bounds().getCenter() - pos).lenSquared();
            if (dist < minDist) {
                minDist = dist;
                best = leaf;
            }
        }
    });

    return best;
}

NavNode* NavSvo::findNode(const Point3F& pos, bool testOccupied) const
{
   NavNode* node = NULL;
   if (mSpatialIndex.tryGetValue(pos, node)) {
      if (!testOccupied || (node && !node->data.occupied))
         return node;
   }
   return getNearestNode(pos, testOccupied);
}

void NavSvo::markOccupiedRecursive(NavNode* node, const AIInfo* proxy)
{
   if (!node) return;
   SphereF sphere(proxy->mPosition, proxy->mRadius);
   const Box3F& box = node->bounds();

   // If the sphereintersects the box, mark all descendants as occupied
   if (box.isOverlapped(sphere)) {
      if (node->isLeaf()) {
         node->setOccupied(true);
      }
      else {
         for (U32 i = 0; i < node->getNumChildren(); ++i)
            markOccupiedRecursive(node->getChild(i), proxy);
      }
   }
}

// Helper: propagate occupancy up the tree
void NavSvo::propagateOccupied(NavNode* node)
{
    if (node->isLeaf())
        return;

    bool allOccupied = true;
    for (U32 i = 0; i < node->getNumChildren(); ++i) {
        NavNode* child = node->getChild(i);
        propagateOccupied(child);
        if (!child->data.occupied)
            allOccupied = false;
    }
    node->setOccupied(allOccupied);
}

void NavSvo::markOccupied(const Vector<AIInfo*>& proxies)
{
    if (!mRoot) return;

    // 1. Clear all occupancy
    mRoot->forEachLeafAs<NavNode>([](NavNode* n) { n->setOccupied(false); });

    // 2. Mark leaves overlapped by any proxy
    for (const AIInfo* proxy : proxies) {
        SphereF sphere(proxy->mPosition, proxy->mRadius);
        mRoot->forEachLeafAs<NavNode>([&](NavNode* leaf) {
            if (leaf->bounds().isOverlapped(sphere))
                leaf->setOccupied(true);
        });
    }

    // 3. Propagate occupied status up the tree
    propagateOccupied(mRoot);
}

void NavSvo::clearOccupied()
{
   if (!mRoot) return;
   mRoot->forEachLeafAs<NavNode>([](NavNode* n) { n->setOccupied(false); });
}

bool NavSvo::findPath(const Point3F& start, const Point3F& end, Vector<Point3F>& outPath)
{
   outPath.clear();

   // 1. Map start/end to SVO leaf nodes using unified lookup
   NavNode* startNode = findNode(start, false);
   NavNode* endNode = findNode(end, false);
   if (!startNode || !endNode)
      return false;

   if (startNode == endNode)
   {
      outPath.push_back(start);
      outPath.push_back(end);
      NavPath* path = new NavPath();
      path->mMesh = this;
      path->mFrom = start;
      path->mFromSet = true;
      path->mTo = end;
      path->mToSet = true;
      path->setPathPoints(outPath);
      path->setStatus(DT_SUCCESS);
      if (!path->registerObject())
      {
         delete path;
         return false;
      }
      return path->success();
   }
   // 2. A* pathfinding using only Torque containers and forEachLeaf for closed set
   struct NodeRecord
   {
      const NavNode* node;
      const NavNode* parent;
      F32 gCost;
      F32 fCost;
   };

   Vector<NodeRecord> openList;
   Vector<const NavNode*> closedSet;

   F32 hStart = (startNode->bounds().getCenter() - endNode->bounds().getCenter()).len();
   openList.push_back({ startNode, NULL, 0.0f, hStart });

   bool found = false;
   const NavNode* goalNode = NULL;

   auto isClosed = [&](const NavNode* n) -> bool {
      for (U32 i = 0; i < closedSet.size(); i++)
         if (closedSet[i] == n)
            return true;
      return false;
   };

   while (!openList.empty())
   {
      // Find node with lowest fCost in openList
      S32 bestIdx = 0;
      F32 bestFCost = openList[0].fCost;
      for (S32 i = 1; i < openList.size(); ++i)
      {
         if (openList[i].fCost < bestFCost)
         {
            bestFCost = openList[i].fCost;
            bestIdx = i;
         }
      }
      NodeRecord currentRec = openList[bestIdx];
      openList.erase(bestIdx);
      closedSet.push_back(currentRec.node);

      if (currentRec.node == endNode)
      {
         found = true;
         goalNode = currentRec.node;
         break;
      }

      // Expand neighbors
      for (U32 i = 0; i < currentRec.node->data.neighbors.size(); ++i)
      {
         NavNode* neighbor = currentRec.node->data.neighbors[i];
         // Allow start and end nodes to be occupied, but not other nodes
         if ((neighbor->data.occupied && neighbor != endNode && neighbor != startNode) ||
             isClosed(neighbor))
            continue;

         // Check if neighbor is in openList
         S32 openIdx = -1;
         for (S32 j = 0; j < openList.size(); ++j)
         {
            if (openList[j].node == neighbor)
            {
               openIdx = j;
               break;
            }
         }

         F32 tentative_gScore = currentRec.gCost + (neighbor->bounds().getCenter() - currentRec.node->bounds().getCenter()).len();
         F32 h = (neighbor->bounds().getCenter() - endNode->bounds().getCenter()).len();
         F32 f = tentative_gScore + h;

         if (openIdx == -1)
         {
            openList.push_back({ neighbor, currentRec.node, tentative_gScore, f });
         }
         else if (tentative_gScore < openList[openIdx].gCost)
         {
            openList[openIdx].gCost = tentative_gScore;
            openList[openIdx].parent = currentRec.node;
            openList[openIdx].fCost = f;
         }
      }
   }

   if (!found)
   {
      NavPath* path = new NavPath();
      path->mMesh = this;
      path->mFrom = start;
      path->mFromSet = true;
      path->mTo = end;
      path->mToSet = true;
      path->setPathPoints(outPath);
      path->setStatus(DT_FAILURE);
      if (!path->registerObject())
         delete path;
      return false;
   }

   // 3. Path reconstruction
   Vector<const NavNode*> pathNodes;
   const NavNode* node = goalNode;
   const NavNode* parent = NULL;
   do
   {
      pathNodes.push_back(node);
      parent = NULL;
      for (U32 i = 0; i < closedSet.size(); ++i)
      {
         if (closedSet[i] == node)
         {
            for (U32 j = 0; j < openList.size(); j++)
            {
               if (openList[j].node == node)
               {
                  parent = openList[j].parent;
                  break;
               }
            }
            break;
         }
      }
      node = parent;
   } while (node);

   // Path is from goal to start, so reverse
   for (S32 i = pathNodes.size() - 1; i >= 0; --i)
      outPath.push_back(pathNodes[i]->bounds().getCenter());

   // Optionally, set the first/last point to the actual start/end
   if (!outPath.empty())
   {
      outPath.first() = start;
      outPath.last() = end;
   }

   // 4. Create and register a NavPath object using public methods
   NavPath* path = new NavPath();
   path->mMesh = this;
   path->mFrom = start;
   path->mFromSet = true;
   path->mTo = end;
   path->mToSet = true;
   path->setPathPoints(outPath);
   path->setStatus(outPath.size() > 1 ? DT_SUCCESS : DT_FAILURE);

   if (!path->registerObject())
   {
      delete path;
      return false;
   }

   return path->success();
}

// debugging
void NavSvo::prepRenderImage(SceneRenderState* state)
{
   ObjectRenderInst* ri = state->getRenderPass()->allocInst<ObjectRenderInst>();
   ri->renderDelegate.bind(this, &NavSvo::render);
   ri->type = RenderPassManager::RIT_Object;
   ri->translucentSort = true;
   ri->defaultKey = 1;
   state->getRenderPass()->addInst(ri);
}

void NavSvo::render(ObjectRenderInst* ri, SceneRenderState* state, BaseMatInstance* overrideMat)
{
   if (!mRender || overrideMat || state->isReflectPass())
      return;

   NetObject* no = getServerObject();
   if (no)
   {
      NavSvo* serverNav = static_cast<NavSvo*>(no);
      serverNav->debugRender();
   }
}

void NavSvo::renderLeaf(const NavNode* node, GFXDrawUtil* drawUtil)
{
   const Box3F& box = node->bounds();
   Point3F extents = box.getExtents();
   GFXStateBlockDesc desc;
   desc.setBlend(true);
   ColorI color(0, 255, 0, 128);
   desc.fillMode = GFXFillSolid;
   drawUtil->drawCube(desc, box, color);
}

void NavSvo::debugRender()
{
   ColorI seedcolor(255, 0, 255, 255);
   ColorI clampedSeedColor(255, 0, 0, 255);
   ColorI gridColor(128, 128, 128, 64);
   ColorI boxColor(0, 128, 255, 255);

    GFXTransformSaver saver;
    GFXDrawUtil* drawUtil = GFX->getDrawUtil();

    // Draw seed position crosses
    Point3F rawSeed = getPosition() + mSeedPos;
    F32 size = 0.5f;
    drawUtil->drawLine(rawSeed + Point3F(-size, 0, 0), rawSeed + Point3F(size, 0, 0), seedcolor);
    drawUtil->drawLine(rawSeed + Point3F(0, -size, 0), rawSeed + Point3F(0, size, 0), seedcolor);
    drawUtil->drawLine(rawSeed + Point3F(0, 0, -size), rawSeed + Point3F(0, 0, size), seedcolor);

    Point3F snappedSeed = getWorldSeedPos();
    drawUtil->drawLine(snappedSeed + Point3F(-size, 0, 0), snappedSeed + Point3F(size, 0, 0), clampedSeedColor);
    drawUtil->drawLine(snappedSeed + Point3F(0, -size, 0), snappedSeed + Point3F(0, size, 0), clampedSeedColor);
    drawUtil->drawLine(snappedSeed + Point3F(0, 0, -size), snappedSeed + Point3F(0, 0, size), clampedSeedColor);

    // Grid and bounds
    const Box3F& bounds = getWorldBox();
    Point3F min = bounds.minExtents;
    Point3F max = bounds.maxExtents;
    F32 step = mFidelity > 0.0f ? mFidelity : 1.0f;

    // Draw grid lines, skipping those on the external bounds
    for (F32 y = min.y; y <= max.y; y += step)
    {
        for (F32 z = min.z; z <= max.z; z += step)
        {
            // Only draw as grid if not on the bounds
            if (y != min.y && y != max.y && z != min.z && z != max.z)
                drawUtil->drawLine(Point3F(min.x, y, z), Point3F(max.x, y, z), gridColor);
        }
    }
    for (F32 x = min.x; x <= max.x; x += step)
    {
        for (F32 z = min.z; z <= max.z; z += step)
        {
            if (x != min.x && x != max.x && z != min.z && z != max.z)
                drawUtil->drawLine(Point3F(x, min.y, z), Point3F(x, max.y, z), gridColor);
        }
    }
    for (F32 x = min.x; x <= max.x; x += step)
    {
        for (F32 y = min.y; y <= max.y; y += step)
        {
            if (x != min.x && x != max.x && y != min.y && y != max.y)
                drawUtil->drawLine(Point3F(x, y, min.z), Point3F(x, y, max.z), gridColor);
        }
    }

    // Draw external bounding box edges (blue)
    Point3F c[8] = {
        Point3F(min.x, min.y, min.z),
        Point3F(max.x, min.y, min.z),
        Point3F(max.x, max.y, min.z),
        Point3F(min.x, max.y, min.z),
        Point3F(min.x, min.y, max.z),
        Point3F(max.x, min.y, max.z),
        Point3F(max.x, max.y, max.z),
        Point3F(min.x, max.y, max.z)
    };
    drawUtil->drawLine(c[0], c[1], boxColor);
    drawUtil->drawLine(c[1], c[2], boxColor);
    drawUtil->drawLine(c[2], c[3], boxColor);
    drawUtil->drawLine(c[3], c[0], boxColor);

    drawUtil->drawLine(c[4], c[5], boxColor);
    drawUtil->drawLine(c[5], c[6], boxColor);
    drawUtil->drawLine(c[6], c[7], boxColor);
    drawUtil->drawLine(c[7], c[4], boxColor);

    drawUtil->drawLine(c[0], c[4], boxColor);
    drawUtil->drawLine(c[1], c[5], boxColor);
    drawUtil->drawLine(c[2], c[6], boxColor);
    drawUtil->drawLine(c[3], c[7], boxColor);

    // Only render SVO leaves if generated
    if (!mRoot)
        return;

    mRoot->forEachLeafAs<NavNode>([&](const NavNode* node)
    {
        renderLeaf(node, drawUtil);
    });
}

//serialization
//networking
U32 NavSvo::packUpdate(NetConnection* conn, U32 mask, BitStream* stream)
{
   U32 retMask = Parent::packUpdate(conn, mask, stream);
   if (stream->writeFlag(mHasDirtyData))
   {
      stream->write(mSeedPos.x);
      stream->write(mSeedPos.y);
      stream->write(mSeedPos.z);
      stream->write(mFidelity);
      stream->writeString(mDataFileName);
      stream->writeFlag(mRender);
      mHasDirtyData = false;
   }
   return retMask;
}

void NavSvo::unpackUpdate(NetConnection* conn, BitStream* stream)
{
   Parent::unpackUpdate(conn, stream);
   if (stream->readFlag())
   {
      stream->read(&mSeedPos.x);
      stream->read(&mSeedPos.y);
      stream->read(&mSeedPos.z);
      stream->read(&mFidelity);
      char fileNameBuffer[256];
      stream->readString(fileNameBuffer);
      mDataFileName = StringTable->insert(fileNameBuffer);
      mRender = stream->readFlag();
      //enough info to replicate on the client side or just read a file
   }
}

//binary file
// Serialize this node and its subtree to a stream
void NavNode::writeToStream(Stream& stream) const
{
   // Write node data
   mathWrite(stream, data.bounds);
   // stream.write(data.traversable); // removed
   // Write number of children
   U32 numChildren = this->getNumChildren();
   stream.write(numChildren);
   // Recursively write children
   for (U32 i = 0; i < numChildren; ++i)
   {
      NavNode* child = const_cast<NavNode*>(getChild(i));
      child->writeToStream(stream);
   }
}

// Deserialize this node and its subtree from a stream
void NavNode::readFromStream(Stream& stream, U32 depth)
{
    if (depth > 64) {
        Con::errorf("NavNode::readFromStream - Aborting: tree depth exceeds 64 (possible corrupt file)");
        return;
    }
    // Read node data
    mathRead(stream, &data.bounds);
    // stream.read(&data.traversable); // removed
    // Read number of children
    U32 numChildren;
    stream.read(&numChildren);
    // Recursively read children
    for (U32 i = 0; i < numChildren; ++i)
    {
        NavNode* child = new NavNode();
        child->readFromStream(stream, depth + 1);
        this->addChild(child);
    }
}

void NavSvo::saveToFile()
{
   if (!mDataFileName || !mDataFileName[0])
   {
      Con::errorf("NavSvo::saveToFile - No filename specified!");
      return;
   }
   FileStream stream;
   if (!stream.open(mDataFileName, Torque::FS::File::Write))
   {
      Con::errorf("NavSvo::saveToFile - Failed to open file: %s", mDataFileName);
      return;
   }
   U32 version = 1;
   stream.write(version);

   // Write SVO tree
   if (mRoot)
      mRoot->writeToStream(stream);

   stream.close();
   Con::printf("NavSvo: SVO saved to %s", mDataFileName);
}

bool NavSvo::loadFromFile()
{
   if (!mDataFileName || !mDataFileName[0])
   {
      Con::errorf("NavSvo::loadFromFile - No filename specified!");
      return false;
   }
   FileStream stream;
   if (!stream.open(mDataFileName, Torque::FS::File::Read))
   {
      Con::errorf("NavSvo::loadFromFile - Failed to open file: %s", mDataFileName);
      return false;
   }
   U32 version;
   stream.read(&version);

   clear();
   mRoot = new NavNode();
   mRoot->readFromStream(stream, 0);

   stream.close();
   Con::printf("NavSvo: SVO loaded from %s", mDataFileName);
   return true;
}

//console methods
DefineEngineMethod(NavSvo, saveToFile, void, (), , "Save the SVO navmesh to disk.")
{
   object->saveToFile();
}
DefineEngineMethod(NavSvo, loadFromFile, void, (), , "Load the SVO navmesh from disk.")
{
   object->loadFromFile();
}

// Debug: Print info about the seed node
void NavSvo::debugSeedNode() const
{
    Point3F worldSeed = getWorldSeedPos();
    NavNode* seedNode = findNode(worldSeed, false);
    if (seedNode)
    {
        Con::printf("[SVO DEBUG] Seed node found at center: %f %f %f",
            seedNode->bounds().getCenter().x,
            seedNode->bounds().getCenter().y,
            seedNode->bounds().getCenter().z);
    }
    else
    {
        Con::printf("[SVO DEBUG] Seed node NOT found for position: %f %f %f",
            worldSeed.x, worldSeed.y, worldSeed.z);
    }
}

// Debug: Print total leaf count
void NavSvo::debugLeafCount(const char* stage) const
{
    U32 count = 0;
    if (mRoot)
        mRoot->forEachLeafAs<NavNode>([&](const NavNode*) { ++count; });
    Con::printf("[SVO DEBUG] Leaf count %s: %u", stage, count);
}
