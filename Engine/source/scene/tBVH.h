#pragma once

#ifndef _TBVH_H_
#define _TBVH_H_

#ifndef _MMATH_H_
#include "math/mMath.h"
#endif

#ifndef _SCENEOBJECT_H_
#include "scene/sceneObject.h"
#endif

#ifndef _COLLISION_H_
#include "collision/collision.h"
#endif

#ifndef _TREEOBJECT_H_
#include "core/util/treeObject.h"
#endif

#ifndef _PLATFORM_THREADS_MUTEX_H_
#include "platform/threads/mutex.h"
#endif
// Forward declaration
class BVHNode;

/// <summary>
/// Proxy interface for objects that can be inserted into the BVH.
/// Derived classes must implement bounds calculation and ray casting.
/// </summary>
class BVHProxy
{
public:
   virtual ~BVHProxy() = default;

   virtual Box3F getBounds() const = 0;
   virtual BVHNode* getBVHNode() const = 0;
   virtual bool castRay(const Point3F& start, const Point3F& end, RayInfo* info) const = 0;
   virtual bool castRayRendered(const Point3F& start, const Point3F& end, RayInfo* info) const = 0;

   bool isGlobalBounds() const { return mGlobalBounds; }
   void setGlobalBounds(bool isGlobal) { mGlobalBounds = isGlobal; }

protected:
   bool mGlobalBounds = false;
};

struct BVHData
{
   Box3F bounds = Box3F::Invalid;
   BVHProxy* object = NULL;
};

class BVHNode : public TreeNode<BVHData, false>
{
   bool mBoundsDirty = false;
   enum Direction { LEFT = 0, RIGHT = 1 };
public:
   void markDirty() { mBoundsDirty = true; }
   bool isDirty() const { return mBoundsDirty; }
   void clearDirty() { mBoundsDirty = false; }

   BVHNode()
   {
      increment(2);
      (*this)[LEFT] = NULL;
      (*this)[RIGHT] = NULL;
   }
   BVHNode(const Vector<TreeNode<BVHData, false>*>& children, BVHNode* p = NULL)
      : TreeNode<BVHData, false>(children, p), mBoundsDirty(false) {}

   BVHNode* getParent() const { return parent ? static_cast<BVHNode*>(parent) : NULL; }

   BVHNode* getChild(U32 i) { return getChildAs<BVHNode>(i); }
   const BVHNode* getChild(U32 i) const { return getChildAs<BVHNode>(i); }
   Vector<BVHNode*> getChildren() { return getChildrenAs<BVHNode>(); }
   Vector<const BVHNode*> getChildren() const { return getChildrenAs<const BVHNode>(); }

   bool isLeaf() const 
   { 
#ifdef TORQUE_DEBUG
      bool hasLeft = getChild(LEFT) != NULL;
      bool hasRight = getChild(RIGHT) != NULL;
      if ((hasLeft || hasRight) && data.object != NULL)
      {
         AssertFatal(false, "BVHNode::isLeaf - Internal node has non-NULL object pointer (corruption detected)");
         return false;
      }
#endif
      return ((*this)[LEFT] == NULL && (*this)[RIGHT] == NULL);
   }

   // Validate this node's state
   bool isValid() const
   {
      if (!isValidTree())
         return false;

      bool hasLeft = (*this)[LEFT] != NULL;
      bool hasRight = (*this)[RIGHT] != NULL;
      bool hasChildren = (hasLeft || hasRight);
      bool hasObject = (data.object != NULL);

      if (hasChildren && hasObject)
         return false;  // Internal node with object pointer

      if (!hasChildren && !hasObject)
         return false;  // Leaf node without object (unless being deleted)

      return true;
   }

   // Data accessors
   const Box3F& bounds() const { return data.bounds; }
   void setBounds(const Box3F& box) { data.bounds = box; }
   BVHProxy* object() const { return data.object; }
   void setObject(BVHProxy* obj) { data.object = obj; }
};


class BVH
{
public:
   using Node = BVHNode;

   struct CollisionPair
   {
      BVHProxy* a;
      BVHProxy* b;
   };

   enum Direction { LEFT = 0, RIGHT = 1 };
   /// <summary>
   /// RAII helper to track active queries and prevent tree corruption.
   /// Thread-safe for use across rendering and physics threads.
   /// Usage: BVH::QueryScope scope(bvh);
   /// </summary>
   class QueryScope
   {
      BVH* mBVH;
   public:
      QueryScope(BVH* bvh) : mBVH(bvh)
      {
         if (mBVH)
         {
            Mutex::lockMutex(mBVH->mMutex);
            mBVH->mActiveQueryCount++;
            Mutex::unlockMutex(mBVH->mMutex);
         }
      }

      ~QueryScope()
      {
         if (mBVH)
         {
            Mutex::lockMutex(mBVH->mMutex);
            mBVH->mActiveQueryCount--;
            Mutex::unlockMutex(mBVH->mMutex);
         }
      }

      // Non-copyable
      QueryScope(const QueryScope&) = delete;
      QueryScope& operator=(const QueryScope&) = delete;
   };

private:
   Node* mRoot = NULL;
   Vector<Node*> mGlobalObjects;

   // Thread-safe query counter for multi-threaded physics
   U32 mActiveQueryCount = 0;
   void* mMutex;

   static constexpr F32 MARGIN = POINT_EPSILON;
   static constexpr U32 MAX_TREE_DEPTH = 32;
   static constexpr F32 ROTATION_THRESHOLD = 0.9f;

public:
   BVH() : mActiveQueryCount(0) { mMutex = Mutex::createMutex(); }

   ~BVH()
   {
      // Wait for active queries to complete
      while (true)
      {
         Mutex::lockMutex(mMutex);
         if (mActiveQueryCount == 0)
         {
            Mutex::unlockMutex(mMutex);
            break;
         }
         Mutex::unlockMutex(mMutex);
         Platform::sleep(1);  // Yield briefly
      }

      if (mRoot)
         destroyRecursive(mRoot);

      for (Node* node : mGlobalObjects)
      {
         if (node && node->object())
            delete node->object();
         delete node;
      }
      mGlobalObjects.clear();

      Mutex::destroyMutex(mMutex);
   }

   inline Node* getRoot() const
   {
      MutexHandle handle;
      handle.lock(mMutex);
      Node* root = mRoot;
      // handle unlocks automatically
      return root;
   }

   inline Vector<Node*> getGlobalObjects() const
   {
      MutexHandle handle;
      handle.lock(mMutex);
      Vector<Node*> copy = mGlobalObjects;
      return copy;
   }

   inline bool hasRoot() const
   {
      MutexHandle handle;
      handle.lock(mMutex);
      bool has = (mRoot != NULL);
      return has;
   }

   inline bool isQueryActive() const
   {
      MutexHandle handle;
      handle.lock(mMutex);
      bool active = (mActiveQueryCount > 0);
      return active;
   }

   //------------------------------------------------
   // CORE API
   //------------------------------------------------
   // Creates a new internal BVH node with children, using the BVHNode constructor.
   Node* createNode(const Vector<Node*>& children, Node* parent = NULL)
   {
      // Convert Vector<Node*> to Vector<TreeNode<BVHData,false>*> for the BVHNode constructor
      Vector<TreeNode<BVHData,false>*> baseChildren;
      baseChildren.setSize(children.size());
      for (U32 i = 0; i < children.size(); ++i)
         baseChildren[i] = static_cast<TreeNode<BVHData, false>*>(children[i]);

      Node* internal = new Node(baseChildren, parent);

      // Set bounds based on children
      if (children.size() == 2)
         internal->setBounds(mergeBounds(children[LEFT]->bounds(), children[RIGHT]->bounds()));
      else if (children.size() == 1)
         internal->setBounds(children[LEFT]->bounds());
      else
         internal->setBounds(Box3F::Invalid);

      internal->setObject(NULL);
      return internal;
   }

   /// @param obj Proxy object to insert. BVH takes ownership of the PROXY object
   ///            (will delete proxy on removeLeaf() or destruction).
   /// @note The WRAPPED object (e.g., SceneObject) must remain valid and is NOT
   ///       owned by BVH. Use deleteNotify to ensure proper cleanup order.
   inline Node* createLeaf(BVHProxy* obj)
   {
      if (!obj) return NULL;

      Node* node = new Node();
      node->setBounds(obj->getBounds());
      node->setObject(obj);
      return node;
   }

   void insertLeaf(Node* leaf)
   {
      AssertFatal(leaf != NULL, "BVH::insertLeaf - Attempted to insert NULL leaf");

      MutexHandle handle;
      handle.lock(mMutex);

      // Handle global objects separately
      if (isGlobalLeaf(leaf))
      {
         mGlobalObjects.push_back_unique(leaf);
         return;
      }

      // First insertion
      if (!mRoot)
      {
         mRoot = leaf;
         leaf->nullParent();
#ifdef TORQUE_DEBUG
         AssertFatal(mRoot->isValidTree(), "BVH::insertLeaf - Tree is invalid after first insertion");
#endif
         return;
      }

      // Find best sibling
      Node* sibling = chooseBestSibling(mRoot, leaf->bounds());
      if (!sibling)
      {
         sibling = mRoot;
      }

      // Create new parent node using createNode
      Vector<Node*> children;
      children.push_back(sibling);
      children.push_back(leaf);
      Node* oldParent = sibling->getParent();
      Node* newParent = createNode(children);

      // Attach new parent to the tree
      if (oldParent)
      {
         oldParent->replaceChild(sibling, newParent);
      }
      else
      {
         mRoot = newParent;
      }

      refitUpwards(newParent);
      rotateUpwards(newParent);

#ifdef TORQUE_DEBUG
      AssertFatal(newParent->isValid(), "BVH::insertLeaf - New parent node is invalid after insertion");
      AssertFatal(mRoot->isValidTree(), "BVH::insertLeaf - Tree is invalid after insertion");
#endif
   }

   void removeLeaf(Node* leaf)
   {
      AssertFatal(leaf != NULL, "BVH::removeLeaf - Attempted to remove NULL leaf");

      MutexHandle handle;
      handle.lock(mMutex);

      // Prevent double removal: if leaf is not in the tree and not root/global, do nothing
      if (!leaf->getParent() && !isNonChildNode(leaf))
         return;

      // Handle global objects
      if (isGlobalLeaf(leaf))
      {
         for (U32 i = 0; i < mGlobalObjects.size(); i++)
         {
            if (mGlobalObjects[i] == leaf)
            {
               mGlobalObjects.erase_fast(i);
               return;
            }
         }
         return;
      }

      // Handle root
      if (leaf == mRoot)
      {
         mRoot = NULL;
         leaf->nullParent();
#ifdef TORQUE_DEBUG
         AssertFatal(leaf->getParent() == NULL, "BVH::removeLeaf - Root leaf should have NULL parent");
#endif
         return;
      }

      Node* parent = leaf->getParent();
      AssertFatal(parent != NULL, "BVH::removeLeaf - Leaf has no parent (tree corruption detected)");
      AssertFatal(parent->getChild(LEFT) == leaf || parent->getChild(RIGHT) == leaf,
         "BVH::removeLeaf - Parent does not reference leaf as a child");

      Direction siblingDir = (parent->getChild(LEFT) == leaf) ? RIGHT : LEFT;
      Node* sibling = (Node*)parent->getChild(siblingDir);

      AssertFatal(sibling != NULL, "BVH::removeLeaf - Parent node missing sibling (tree corruption detected)");
      AssertFatal(sibling != leaf, "BVH::removeLeaf - Sibling is the same as leaf (tree corruption detected)");
      AssertFatal(sibling->getParent() == parent, "BVH::removeLeaf - Sibling's parent is not correct");

      Node* grandParent = parent->getParent();
      if (grandParent) {
         AssertFatal(grandParent->getChild(LEFT) == parent || grandParent->getChild(RIGHT) == parent,
            "BVH::removeLeaf - Grandparent does not reference parent as a child");

         sibling->setParent(grandParent);

         grandParent->replaceChild(parent, sibling);
         refitUpwards(grandParent);
         if (grandParent && grandParent->getChild(LEFT) && grandParent->getChild(RIGHT))
            rotateUpwards(grandParent);
#ifdef TORQUE_DEBUG
         AssertFatal(grandParent->isValid(), "BVH::removeLeaf - Tree is invalid after refit/rotate");
         AssertFatal(sibling->isValidTree(), "BVH::removeLeaf - Sibling subtree is invalid after removal");
#endif
      } else {
         mRoot = sibling;
         sibling->setParent(NULL);
#ifdef TORQUE_DEBUG
         AssertFatal(mRoot->isValidTree(), "BVH::removeLeaf - Root is invalid after removal");
#endif
      }

      for (U32 i = 0; i < 2; ++i)
         parent->nullChild(i);

      parent->compact();

#ifdef TORQUE_DEBUG
      AssertFatal(leaf->getParent() == NULL, "BVH::removeLeaf - Leaf parent not cleared");
      AssertFatal(sibling->getParent() == (grandParent ? grandParent : NULL), "BVH::removeLeaf - Sibling parent not set correctly");
      if (mRoot)
         AssertFatal(mRoot->isValidTree(), "BVH::removeLeaf - Tree is invalid after removal");
#endif

      // handle unlocks automatically

      delete parent;
   }

   void updateLeaf(Node* leaf)
   {
      AssertFatal(leaf != NULL, "BVH::updateLeaf - Attempted to update NULL leaf");
      AssertFatal(leaf->isLeaf(), "BVH::updateLeaf - Node is not a leaf");
      BVHProxy* obj = leaf->object();
      AssertFatal(obj != NULL, "BVH::updateLeaf - Leaf node has NULL object");

      MutexHandle handle;
      handle.lock(mMutex);

      // Handle global objects
      if (obj->isGlobalBounds())
      {
         leaf->setBounds(obj->getBounds());
         return;
      }

      Box3F newBounds = obj->getBounds();
      Box3F expanded = padBounds(leaf->bounds());

      // Skip if still contained
      if (expanded.isContained(newBounds))
      {
         return;
      }

      Box3F expandedNewBounds = padBounds(newBounds);

      leaf->setBounds(expandedNewBounds);

      bool queryActive = (mActiveQueryCount > 0);

      if (queryActive)
      {
         // OPTIMIZED: Only refit path from leaf to root
         refitPathToRoot(leaf);
         return;
      }

      // Safe to restructure - no active queries
      Node* parent = leaf->getParent();
      if (parent)
      {
         // Check if we need to restructure
         Box3F parentBounds = parent->bounds();
         parentBounds.minExtents -= Point3F(MARGIN * 2, MARGIN * 2, MARGIN * 2);
         parentBounds.maxExtents += Point3F(MARGIN * 2, MARGIN * 2, MARGIN * 2);

         // If still fits reasonably in parent, just refit upwards
         if (parentBounds.isContained(expandedNewBounds))
         {
            refitUpwards(parent);
            return;
         }
      }

      // Need full re-insertion (unlock during recursive calls to prevent deadlock)
      handle.unlock();
      removeLeaf(leaf);
      insertLeaf(leaf);
   }

   void updateLeaves(const Vector<Node*>& leaves)
   {
      AssertFatal(!leaves.empty(), "BVH::updateLeaves - Empty leaves vector");

      MutexHandle handle;
      handle.lock(mMutex);
      bool queryActive = (mActiveQueryCount > 0);

      // Phase 1: Update bounds for all dirty leaves
      Vector<Node*> dirtyLeaves;
      dirtyLeaves.reserve(leaves.size());

      for (U32 i = 0; i < leaves.size(); i++)
      {
         Node* leaf = leaves[i];
         AssertFatal(leaf != NULL, "BVH::updateLeaves - NULL leaf in leaves vector");
         AssertFatal(leaf->isLeaf(), "BVH::updateLeaves - Node is not a leaf");
         BVHProxy* obj = leaf->object();
         AssertFatal(obj != NULL, "BVH::updateLeaves - Leaf node has NULL object");

         if (obj->isGlobalBounds())
         {
            leaf->setBounds(obj->getBounds());
            continue;
         }

         Box3F newBounds = obj->getBounds();
         Box3F expanded = leaf->bounds();
         expanded.minExtents -= Point3F(MARGIN, MARGIN, MARGIN);
         expanded.maxExtents += Point3F(MARGIN, MARGIN, MARGIN);

         if (expanded.isContained(newBounds))
            continue;

         Box3F expandedNewBounds = newBounds;
         expandedNewBounds.minExtents -= Point3F(MARGIN, MARGIN, MARGIN);
         expandedNewBounds.maxExtents += Point3F(MARGIN, MARGIN, MARGIN);

         leaf->setBounds(expandedNewBounds);
         dirtyLeaves.push_back(leaf);
      }

      if (dirtyLeaves.empty())
      {
         handle.unlock();
         return;
      }

      // Phase 2: If query is active, just refit paths
      if (queryActive)
      {
         Map<Node*, bool> ancestorSet;
         for (U32 i = 0; i < dirtyLeaves.size(); i++)
         {
            Node* node = dirtyLeaves[i]->getParent();
            while (node)
            {
               ancestorSet.insert(node, true);
               node = node->getParent();
            }
         }

         Vector<Node*> sortedAncestors;
         sortedAncestors.reserve(ancestorSet.size());
         for (Map<Node*, bool>::Iterator iter = ancestorSet.begin(); iter != ancestorSet.end(); ++iter)
            sortedAncestors.push_back(iter->key);

         sortedAncestors.sort(compareNodesByDepth);

         for (U32 i = 0; i < sortedAncestors.size(); i++)
            refitNode(sortedAncestors[i]);

         handle.unlock();
         return;
      }

      // Phase 3: Check which leaves need reinsertion
      Vector<Node*> needsReinsertion;
      needsReinsertion.reserve(dirtyLeaves.size());

      for (U32 i = 0; i < dirtyLeaves.size(); i++)
      {
         Node* leaf = dirtyLeaves[i];
         Node* parent = leaf->getParent();

         if (!parent)
            continue;

         Box3F expandedNewBounds = leaf->bounds();
         Box3F parentBounds = parent->bounds();
         parentBounds.minExtents -= Point3F(MARGIN * 2, MARGIN * 2, MARGIN * 2);
         parentBounds.maxExtents += Point3F(MARGIN * 2, MARGIN * 2, MARGIN * 2);

         if (!parentBounds.isContained(expandedNewBounds))
         {
            needsReinsertion.push_back(leaf);
         }
      }

      // Phase 4: Batch refit leaves that don't need reinsertion
      Vector<Node*> refitOnly;
      for (U32 i = 0; i < dirtyLeaves.size(); i++)
      {
         Node* leaf = dirtyLeaves[i];
         bool needsReinsert = false;

         for (U32 j = 0; j < needsReinsertion.size(); j++)
         {
            if (needsReinsertion[j] == leaf)
            {
               needsReinsert = true;
               break;
            }
         }

         if (!needsReinsert)
            refitOnly.push_back(leaf);
      }

      if (!refitOnly.empty())
      {
         Map<Node*, bool> ancestorSet;
         for (U32 i = 0; i < refitOnly.size(); i++)
         {
            Node* node = refitOnly[i]->getParent();
            while (node)
            {
               ancestorSet.insert(node, true);
               node = node->getParent();
            }
         }

         Vector<Node*> sortedAncestors;
         sortedAncestors.reserve(ancestorSet.size());
         for (Map<Node*, bool>::Iterator iter = ancestorSet.begin(); iter != ancestorSet.end(); ++iter)
            sortedAncestors.push_back(iter->key);

         sortedAncestors.sort(compareNodesByDepth);

         for (U32 i = 0; i < sortedAncestors.size(); i++)
            refitNode(sortedAncestors[i]);
      }

      handle.unlock();

      // Phase 5: Reinsert leaves that don't fit (keep locked to prevent races)
      for (U32 i = 0; i < needsReinsertion.size(); i++)
      {
         Node* leaf = needsReinsertion[i];
         removeLeaf(leaf);
         insertLeaf(leaf);
      }
   }

private:
   //------------------------------------------------
   // COST METRICS
   //------------------------------------------------
   inline Box3F padBounds(const Box3F& bounds) const
   {
      Box3F expanded = bounds;
      expanded.minExtents -= Point3F(MARGIN, MARGIN, MARGIN);
      expanded.maxExtents += Point3F(MARGIN, MARGIN, MARGIN);
      return expanded;
   }

   inline Box3F mergeBounds(const Box3F& a, const Box3F& b) const
   {
      if (!a.isValidBox() || !b.isValidBox())
         return Box3F::Invalid;
      Box3F result = a;
      result.extend(b.minExtents);
      result.extend(b.maxExtents);
      return result;
   }


   inline F32 getSurfaceArea(const Box3F& box) const
   {
      if (!box.isValidBox())
         return F32_MAX;  // High cost for invalid bounds to avoid destabilizing rotations
      Point3F extent = box.maxExtents - box.minExtents;
      return 2.0f * (extent.x * extent.y + extent.y * extent.z + extent.z * extent.x);
   }

   inline F32 mergeCost(const Box3F& a, const Box3F& b) const
   {
      return getSurfaceArea(mergeBounds(a, b));
   }

   F32 surfaceAreaCost(const BVHNode* a, const BVHNode* b)
   {
      // Example: merge bounds and compute surface area
      Box3F merged = mergeBounds(a->bounds(), b->bounds());
      return getSurfaceArea(merged);
   }

   static F32 bvhRotationPairCost(const TreeNode<BVHData,false>* a, const TreeNode<BVHData, false>* b)
   {
      const BVHNode* na = static_cast<const BVHNode*>(a);
      const BVHNode* nb = static_cast<const BVHNode*>(b);
      Box3F merged = na->bounds();
      merged.extend(nb->bounds().minExtents);
      merged.extend(nb->bounds().maxExtents);
      Point3F extent = merged.maxExtents - merged.minExtents;
      return 2.0f * (extent.x * extent.y + extent.y * extent.z + extent.z * extent.x);
   }

   // Comparison function for sorting nodes by depth (deepest first)
   static S32 QSORT_CALLBACK compareNodesByDepth(Node* const* a, Node* const* b)
   {
      U32 depthA = (*a)->getDepth();  // Direct access
      U32 depthB = (*b)->getDepth();

      // Sort descending (deepest first for bottom-up processing)
      if (depthA > depthB) return -1;
      if (depthA < depthB) return 1;
      return 0;
   }

   //------------------------------------------------
   // INTERNAL HELPERS
   //------------------------------------------------

   inline bool isGlobalLeaf(Node* leaf) const
   {
      return leaf && leaf->object() && leaf->object()->isGlobalBounds();
   }

   // Utility method to check if a node is the root or a global object
   bool isNonChildNode(Node* node) const
   {
      if (!node) return false;
      if (node == mRoot) return true;
      for (U32 i = 0; i < mGlobalObjects.size(); ++i)
      {
         if (mGlobalObjects[i] == node) return true;
      }
      return false;
   }

   // Helper to refit a single node from its children
   // Returns true if bounds changed, false if unchanged
   inline bool refitNode(Node* node)
   {
      if (!node || node->isLeaf()) return false;

      Vector<Node*> children = node->getChildren();

      Box3F newBounds = Box3F::Invalid;
      if (children.size() == 2)
         newBounds = mergeBounds(children[LEFT]->bounds(), children[RIGHT]->bounds());
      else if (children.size() == 1)
         newBounds = children[LEFT]->bounds();

      if (!newBounds.isValidBox())
         newBounds = Box3F::Invalid;  // Fallback to prevent propagation

      bool changed = !(newBounds.minExtents == node->bounds().minExtents &&
         newBounds.maxExtents == node->bounds().maxExtents);

      node->setBounds(newBounds);
      return changed;
   }

   void destroyRecursive(Node* node)
   {
      AssertFatal(node != NULL, "BVH::destroyRecursive - Attempted to destroy NULL node");

      Vector<BVHNode*> children = node->getChildren();
      for (BVHNode* child : children)
         destroyRecursive(child);

      if (node->object())
         delete node->object();

      for (U32 i = 0; i < node->size(); ++i)
         node->nullChild(i);

      delete node;
   }

   void refitPathToRoot(Node* leaf)
   {
      if (!leaf) return;

      // Start from leaf's parent (leaf already updated)
      Node* node = leaf->getParent();

      while (node)
      {
         refitNode(node);
         node = node->getParent();
      }
   }

   Node* chooseBestSibling(Node* start, const Box3F& leafBounds) const
   {
      Node* node = start;
      F32 bestCost = F32_MAX;
      Node* bestNode = node;

      Vector<Node*> stack;
      stack.push_back(node);

      while (!stack.empty())
      {
         node = stack.last();
         stack.pop_back();

         if (!node) continue;

         F32 directCost = mergeCost(node->bounds(), leafBounds);

         if (directCost < bestCost)
         {
            bestCost = directCost;
            bestNode = node;
         }

         if (!node->isLeaf())
         {
            F32 inheritanceCost = getSurfaceArea(node->bounds());
            F32 minChildCost = directCost + inheritanceCost;

            if (minChildCost < bestCost)
            {
               Vector<Node*> children = node->getChildren();
               for (Node* child : children)
                  stack.push_back(child);
            }
         }
      }

      return bestNode;
   }

   void refitUpwards(Node* node)
   {
      while (node)
      {
         if (!node->isDirty())
            return;  // Early-out: already clean
         
         if (!node->isLeaf())
         {
            if (!refitNode(node))
            {
               node->clearDirty();
               return;
            }
         }
      
         node->clearDirty();
         if (node->getParent())
            node->getParent()->markDirty();
         node = node->getParent();
      }
   }

   void rotateUpwards(Node* node)
   {
      U32 depth = 0;
      Vector<TreeNode<BVHData, false>*> visited;  // Track visited nodes to prevent cycles
      while (node && depth++ < MAX_TREE_DEPTH)
      {
         if (node->isVisited(visited))
            break;  // Cycle detected, stop
         visited.push_back(node);

         if (!node->isLeaf())
            tryRotate(node);
         node = node->getParent();
      }
   }

   bool tryRotate(Node* P)
   {
      if (!P || P->isLeaf() || mActiveQueryCount > 0) return false;
      if (P->size() < 2) return false;

      Vector<TreeNode<BVHData, false>*> baseCandidates;
      U32 candidateCount = TreeNode<BVHData, false>::gatherRotationCandidates(P, baseCandidates, 4);
      if (candidateCount != 4) return false;

      Vector<U32> groupA, groupB;
      F32 bestCost;
      bool found = TreeNode<BVHData, false>::findBestRotationPairing(
         baseCandidates, 2,
         bvhRotationPairCost,
         groupA, groupB, bestCost
      );
      if (!found) return false;

      F32 currentCost = getSurfaceArea(P->bounds());
      if (bestCost >= currentCost * ROTATION_THRESHOLD)
         return false;

      // Collect old children before replacing
      Vector<Node*> oldChildren;
      for (U32 i = 0; i < P->size(); ++i) {
         Node* child = static_cast<Node*>((*P)[i]);
         if (child) oldChildren.push_back(child);
      }

      // Construct children vectors for createNode
      Vector<Node*> leftChildren, rightChildren;
      leftChildren.push_back((Node*)baseCandidates[groupA[LEFT]]);
      leftChildren.push_back((Node*)baseCandidates[groupA[RIGHT]]);
      rightChildren.push_back((Node*)baseCandidates[groupB[LEFT]]);
      rightChildren.push_back((Node*)baseCandidates[groupB[RIGHT]]);

      // Use createNode for new internal nodes
      Node* newLeft = createNode(leftChildren);
      Node* newRight = createNode(rightChildren);

      P->setChild(LEFT, newLeft);
      P->setChild(RIGHT, newRight);

      for (Node* child : oldChildren) {
         if (child != newLeft && child != newRight) {
            // Find the index of the child to nullify the slot
            S32 idx = P->getChildIndex(child);
            delete child;
            if (idx != -1)
               P->nullChild((U32)idx);
         }
      }

      P->setBounds(mergeBounds(newLeft->bounds(), newRight->bounds()));
      P->compact();

#ifdef TORQUE_DEBUG
      AssertFatal(P->isValidTree(), "BVH::tryRotate - Tree structure corrupted after unified rotation");
#endif

      return true;
   }

   // Unified overlap finding - handles all cases (self-collisions, external nodes, etc.)
   void findOverlaps(const Node* n1, const Node* n2, Vector<CollisionPair>& pairs) const
   {
      struct TraversalPair {
         const Node* a;
         const Node* b;
      };

      Vector<TraversalPair> stack;
      stack.reserve(64);  // Pre-allocate reasonable size
      stack.push_back({ n1, n2 });

      while (!stack.empty())
      {
         TraversalPair current = stack.last();
         stack.pop_back();

         const Node* a = current.a;
         const Node* b = current.b;

         if (!a || !b || !a->bounds().isOverlapped(b->bounds()))
            continue;

         if (a->isLeaf() && b->isLeaf())
         {
            if (a != b && a->object() && b->object())
               pairs.push_back({ a->object(), b->object() });
            continue;
         }

         // Expand traversal
         if (a->isLeaf())
         {
            Vector<const Node*> bChildren = b->getChildren();
            for (const Node* child : bChildren)
               stack.push_back({ a, child });
         }
         else if (b->isLeaf())
         {
            Vector<const Node*> aChildren = a->getChildren();
            for (const Node* child : aChildren)
               stack.push_back({ child, b });
         }
         else
         {
            Vector<const Node*> aChildren = a->getChildren();
            Vector<const Node*> bChildren = b->getChildren();
            for (const Node* aChild : aChildren)
               for (const Node* bChild : bChildren)
                  stack.push_back({ aChild, bChild });
         }
      }
   }

   //------------------------------------------------
   // DIAGNOSTIC FUNCTIONS
   //------------------------------------------------

public:

   Vector<CollisionPair> getPotentialCollisionPairs()
   {
      MutexHandle handle;
      handle.lock(mMutex);
      mActiveQueryCount++;
      Node* root = mRoot;
      Vector<Node*> globals = mGlobalObjects;
      handle.unlock();

      Vector<CollisionPair> pairs;

      if (!root || root->isLeaf())
      {
         MutexHandle handle2;
         handle2.lock(mMutex);
         mActiveQueryCount--;
         // handle2 unlocks automatically
         return pairs;
      }

      // Find self-collisions within the tree
      findOverlaps(root, root, pairs);

      // Test global objects against tree
      for (Node* global : globals)
         findOverlaps(global, root, pairs);

      MutexHandle handle3;
      handle3.lock(mMutex);
      mActiveQueryCount--;
      // handle3 unlocks automatically

      return pairs;
   }

   Vector<BVHProxy*> queryRegion(const Box3F& region, bool includeGlobal = true) const
   {
      QueryScope query(const_cast<BVH*>(this));

      MutexHandle handle;
      handle.lock(mMutex);
      Node* root = mRoot;
      Vector<Node*> globals = mGlobalObjects;
      // handle unlocks automatically

      Vector<BVHProxy*> results;
      if (!root && globals.empty())
         return results;

      // Query the tree using stack-based traversal
      if (root)
      {
         Vector<Node*> stack;
         stack.reserve(64);
         stack.push_back(root);

         while (!stack.empty())
         {
            Node* node = stack.last();
            stack.pop_back();

            if (!node || !node->bounds().isOverlapped(region))
               continue;

            if (node->isLeaf())
            {
               if (node->object())
                  results.push_back(node->object());
            }
            else
            {
               if (node->getChild(LEFT)) stack.push_back((Node*)node->getChild(LEFT));
               if (node->getChild(RIGHT)) stack.push_back((Node*)node->getChild(RIGHT));
            }
         }
      }

      if (includeGlobal)
      {
         for (Node* global : globals)
         {
            if (global && global->object())
               results.push_back(global->object());
         }
      }

      // QueryScope handles mActiveQueryCount decrement
      return results;
   }

   BVHProxy* findNearest(const Point3F& point, F32 maxDist = F32_MAX) const
   {
      MutexHandle handle;
      handle.lock(mMutex);
      const_cast<BVH*>(this)->mActiveQueryCount++;
      Node* root = mRoot;
      handle.unlock();

      if (!root)
      {
         MutexHandle handle2;
         handle2.lock(mMutex);
         const_cast<BVH*>(this)->mActiveQueryCount--;
         // handle2 unlocks automatically
         return NULL;
      }

      BVHProxy* nearest = NULL;
      F32 nearestDistSq = maxDist * maxDist;

      Vector<Node*> stack;
      stack.reserve(64);
      stack.push_back(root);

      while (!stack.empty())
      {
         Node* node = stack.last();
         stack.pop_back();

         if (!node)
            continue;

         F32 distSq = node->bounds().getSqDistanceToPoint(point);
         if (distSq > nearestDistSq)
            continue;

         if (node->isLeaf())
         {
            if (node->object())
            {
               const Box3F& box = node->bounds();
               Point3F center = box.getCenter();
               Point3F delta = center - point;
               F32 objDistSq = delta.lenSquared();

               if (objDistSq < nearestDistSq)
               {
                  nearestDistSq = objDistSq;
                  nearest = node->object();
               }
            }
         }
         else
         {
            Node* left = static_cast<Node*>(node->getChild(LEFT));
            Node* right = static_cast<Node*>(node->getChild(RIGHT));

            if (left && right)
            {
               F32 leftDist = left->bounds().getSqDistanceToPoint(point);
               F32 rightDist = right->bounds().getSqDistanceToPoint(point);

               if (leftDist < rightDist)
               {
                  if (rightDist <= nearestDistSq) stack.push_back(right);
                  if (leftDist <= nearestDistSq) stack.push_back(left);
               }
               else
               {
                  if (leftDist <= nearestDistSq) stack.push_back(left);
                  if (rightDist <= nearestDistSq) stack.push_back(right);
               }
            }
            else if (left)
            {
               stack.push_back(left);
            }
            else if (right)
            {
               stack.push_back(right);
            }
         }
      }

      MutexHandle handle3;
      handle3.lock(mMutex);
      const_cast<BVH*>(this)->mActiveQueryCount--;
      // handle3 unlocks automatically

      return nearest;
   }

   //------------------------------------------------
   // QUERY FUNCTIONS
   //------------------------------------------------
#ifdef TORQUE_DEBUG

   /// <summary>
   /// Diagnostic information about the BVH tree structure and health.
   /// Used for debugging and performance analysis.
   /// </summary>
   struct DiagnosticInfo {
      U32 totalNodes = 0;           // Total nodes in tree
      U32 leafNodes = 0;            // Leaf nodes (with objects)
      U32 internalNodes = 0;        // Internal nodes (no objects)
      U32 globalObjects = 0;        // Objects in global list
      U32 maxDepth = 0;             // Maximum tree depth
      U32 minDepth = U32_MAX;       // Minimum leaf depth
      U32 corruptedNodes = 0;       // Nodes with invalid state
      U32 orphanedNodes = 0;        // Nodes with broken parent pointers
      U32 boundsMismatches = 0;     // Parent bounds don't contain children
      F32 avgLeafDepth = 0.0f;      // Average depth of leaf nodes
      F32 totalSurfaceArea = 0.0f;  // Total surface area of all nodes
      F32 avgLeafCost = 0.0f;       // Average surface area of leaves
      
      // Format for console output
      const char* toString() const
      {
         static char buffer[1024];
         dSprintf(buffer, sizeof(buffer),
            "BVH Diagnostics:\n"
            "  Total Nodes: %u (Leaves: %u, Internal: %u, Global: %u)\n"
            "  Depth: Max=%u, Min=%u, AvgLeaf=%.2f\n"
            "  Surface Area: Total=%.2f, AvgLeaf=%.2f\n"
            "  Issues: Corrupted=%u, Orphaned=%u, BoundsMismatch=%u",
            totalNodes, leafNodes, internalNodes, globalObjects,
            maxDepth, minDepth == U32_MAX ? 0 : minDepth, avgLeafDepth,
            totalSurfaceArea, avgLeafCost,
            corruptedNodes, orphanedNodes, boundsMismatches);
         return buffer;
      }
   };

   /// <summary>
   /// Validates the entire BVH tree structure and returns diagnostic information.
   /// Thread-safe and can be called while queries are active.
   /// </summary>
   DiagnosticInfo validateAndDiagnose() const
   {
      DiagnosticInfo info;
      
      Mutex::lockMutex(mMutex);
      Node* root = mRoot;
      U32 globalCount = mGlobalObjects.size();
      Mutex::unlockMutex(mMutex);
      
      info.globalObjects = globalCount;
      
      if (!root)
         return info;
      
      // Validate the tree structure
      validateSubtree(root, 0, NULL, info);
      
      // Calculate averages
      if (info.leafNodes > 0)
      {
         info.avgLeafDepth /= info.leafNodes;
         info.avgLeafCost /= info.leafNodes;
      }

      return info;
   }

private:

   /// <summary>
   /// Recursively validates a subtree and collects diagnostic information.
   /// </summary>
   void validateSubtree(const Node* node, U32 depth, const Node* expectedParent, DiagnosticInfo& info) const
   {
      if (!node) return;
     
      info.totalNodes++;
      info.maxDepth = mMax(info.maxDepth, depth);
      info.totalSurfaceArea += getSurfaceArea(node->bounds());
      
      // Validate parent pointer
      if (node->getParent() != expectedParent)
      {
         info.orphanedNodes++;
         info.corruptedNodes++;
      }
      
      // Validate node-level invariants
      if (!node->isValid())
         info.corruptedNodes++;
     
      if (node->isLeaf())
      {
         info.leafNodes++;
         info.minDepth = mMin(info.minDepth, depth);
         info.avgLeafDepth += depth;
         
        if (node->object())
         {
            F32 leafArea = getSurfaceArea(node->bounds());
            info.avgLeafCost += leafArea;
         }
         else
         {
            // Leaf without object is an error
            info.corruptedNodes++;
         }
      }
      else
      {
         info.internalNodes++;
         
         // Internal node should not have object
        if (node->object())
            info.corruptedNodes++;
         
      const Node* left = node->getChild(LEFT);
      const Node* right = node->getChild(RIGHT);
         
         // Validate that internal nodes have at least one child
         if (!left && !right)
         {
            info.corruptedNodes++;
         }
         
         // Validate bounds contain children
         if (left)
         {
            const Box3F& childBounds = left->bounds();
            if (!node->bounds().isContained(childBounds))
            {
               info.boundsMismatches++;
               info.corruptedNodes++;
            }
            
            validateSubtree(left, depth + 1, node, info);
         }
         
         if (right)
         {
            const Box3F& childBounds = right->bounds();
            if (!node->bounds().isContained(childBounds))
            {
               info.boundsMismatches++;
               info.corruptedNodes++;
            }
            
            validateSubtree(right, depth + 1, node, info);
         }
      }
   }
#endif
};

#endif // !_TBVH_H_

