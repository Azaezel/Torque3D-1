#pragma once

#include "navigation/navMesh.h"
#include "navigation/navPath.h"
#include "math/mBox.h"
#include "core/util/tVector.h"
#include "core/util/tDictionary.h"
#include "core/util/treeObject.h"
#include "core/stream/stream.h"
#include "T3D/AI/AIInfo.h"

class NavNode;

// Data stored in each navigation node (can be extended for SVO or other nav types)
struct NavData
{
    Box3F bounds = Box3F::Invalid;
    bool occupied = false;
    Vector<NavNode*> neighbors;

    bool navigable = false; // stritly for generation use
};

// NavNode is a tree node holding NavData
class NavNode : public TreeNode<NavData>
{
public:
    using TreeNode<NavData>::TreeNode;
    const NavNode* getChild(U32 i) const { return getChildAs<NavNode>(i); }
    NavNode* getChild(U32 i) { return getChildAs<NavNode>(i); }
    Vector<const NavNode*> getChildren() const { return getChildrenAs<NavNode>(); }
    const Box3F& bounds() const { return data.bounds; }
    void setBounds(const Box3F& box) { data.bounds = box; }
    void setNeighbor(NavNode* neighbor, bool add) { (add)? data.neighbors.push_back(neighbor) : data.neighbors.remove(neighbor); }
    void pruneNeighbors();
    void setOccupied(bool occupied) { data.occupied = occupied; }
    void writeToStream(Stream& stream) const;
    void readFromStream(Stream& stream, U32 depth);
    bool pendingDelete;
};

class NavSvo : public NavMesh
{
    typedef NavMesh Parent;
public:
    NavSvo();
    ~NavSvo() override;
    bool onAdd() override;
    void onRemove() override;
    void inspectPostApply() override;

    static void initPersistFields();

    U32 packUpdate(NetConnection* conn, U32 mask, BitStream* stream) override;
    void unpackUpdate(NetConnection* conn, BitStream* stream) override;

    DECLARE_CONOBJECT(NavSvo);
    DECLARE_CATEGORY("Navigation");

    /// Build the sparse voxel octree for the object's bounds using the configured seed and fidelity.
    void buildSVO();
    void rebuildSpatialIndex();

    /// Recursive helpers for octree construction.
    Vector<NavNode*> splitNode(NavNode* parent);
    void buildSVORecursive(NavNode* node, U32 depth);
    void assignNeighbors();
    void pruneAllNeighbors();
    void pruneOrphans(NavNode* start);
    void deleteNodes(Vector<NavNode*>& nodesToDelete);
    void removeNeighborReferences(NavNode* root, NavNode* node);
    bool clusterRaycast(const Point3F& from, const Point3F& to, U32 mask, F32 maxSlopeCos, F32 rayLen, bool isCeiling);
    bool isLeafNavigable(const Box3F& box);

    //inspector callbacks
    static bool _buildNav(void* object, const char* index, const char* data);
    static bool _saveNav(void* object, const char* index, const char* data);

    /// Mark SVO nodes as occupied if their bounds intersect any box in the given set.
    void markOccupiedRecursive(NavNode* node, const AIInfo* proxy);
    void propagateOccupied(NavNode* node);
    void markOccupied(const Vector<AIInfo*>& proxies);
    void clearOccupied();

    /// Find a path from start to end using the SVO (A* not yet implemented).
    bool findPath(const Point3F& start, const Point3F& end, Vector<Point3F>& outPath);

    /// Find the nearest leaf node to a point.
    NavNode* findNearestLeaf(const Point3F& pt) const;

    /// Render the SVO for debugging.
    void prepRenderImage(SceneRenderState* state) override;
    void render(ObjectRenderInst* ri, SceneRenderState* state, BaseMatInstance* overrideMat);
    void renderLeaf(const NavNode* node, GFXDrawUtil* drawUtil);
    void debugRender();
    bool mRender;

    /// Clear the SVO.
    void clear();
    void saveToFile();
    bool loadFromFile();

    void pointToNodeCoord(Point3F* pt);
    NavNode* getNodeContaining(const Point3F& pos, bool testOccupied = false) const; // point is actually within
    NavNode* getNearestNode(const Point3F& pos, bool testOccupied = false) const; //close enough
    /// Fastest node lookup: tries spatial index, falls back to tree walk.
    NavNode* findNode(const Point3F& pos, bool testOccupied = false) const;

    // Debug utilities
    Point3F getWorldSeedPos() const;
    void debugSeedNode() const;
    void debugLeafCount(const char* stage) const;
protected:
   //settings
    Point3F mSeedPos; // Initial floodfill position (seed)
    F32 mFidelity;    // Floodfill step size (voxel size)
    bool mfull3d;;   // Whether to do full 3D floodfill or just x/y axis
    StringTableEntry mDataFileName;

    //actions
    bool mBuildNav; // Trigger to build navmesh
    bool mSaveNav; // Trigger to save navmesh

    //internal SVO root
    NavNode* mRoot;
    bool mHasDirtyData = false;

    // Global spatial index for fast node lookup (center position -> node)
    Map<Point3F, NavNode*> mSpatialIndex;
};
