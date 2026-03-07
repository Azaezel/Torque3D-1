#include "scene/tBVH.h"
#include "platform/platformTimer.h" // For Platform::getRealMilliseconds()

// Mock BVHProxy for testing
class TestProxy : public BVHProxy
{
   Box3F mBounds;
   BVHNode* mNode;
public:
   TestProxy(const Box3F& bounds) : mBounds(bounds), mNode(nullptr) {}
   Box3F getBounds() const override { return mBounds; }
   BVHNode* getBVHNode() const override { return mNode; }
   bool castRay(const Point3F&, const Point3F&, RayInfo*) const override { return false; }
   bool castRayRendered(const Point3F&, const Point3F&, RayInfo*) const override { return false; }
   void setBVHNode(BVHNode* node) { mNode = node; }
   void setBounds(const Box3F& b) { mBounds = b; }
};

static bool runBVHUnitTest(U32 entityCount, U32 globalCount)
{
   BVH bvh;
   Vector<TestProxy*> proxies;
   proxies.reserve(entityCount + globalCount);
   Vector<BVH::Node*> nodes;
   nodes.reserve(entityCount + globalCount);

   MRandomLCG rng(42);

   U32 t0, t1, t2, t3, t4, t5, t6;

   // Insert entityCount random entities
   t0 = Platform::getRealMilliseconds();
   for (U32 i = 0; i < entityCount; ++i)
   {
      Point3F min(rng.randF() * 1000, rng.randF() * 1000, rng.randF() * 1000);
      Point3F max = min + Point3F(1, 1, 1);
      Box3F bounds(min, max);
      TestProxy* proxy = new TestProxy(bounds);
      BVH::Node* node = bvh.createLeaf(proxy);
      proxy->setBVHNode(node);
      bvh.insertLeaf(node);
      proxies.push_back(proxy);
      nodes.push_back(node);
   }
   t1 = Platform::getRealMilliseconds();

   // Insert 100 global objects
   for (U32 i = 0; i < globalCount; ++i)
   {
      Point3F min(rng.randF() * 1000, rng.randF() * 1000, rng.randF() * 1000);
      Point3F max = min + Point3F(10, 10, 10); // Larger bounds for global objects
      Box3F bounds(min, max);
      TestProxy* proxy = new TestProxy(bounds);
      proxy->setGlobalBounds(true); // Mark as global
      BVH::Node* node = bvh.createLeaf(proxy);
      proxy->setBVHNode(node);
      bvh.insertLeaf(node);
      proxies.push_back(proxy);
      nodes.push_back(node);
   }
   t2 = Platform::getRealMilliseconds();

   // Query a region that should contain some entities
   Box3F queryRegion(Point3F(0, 0, 0), Point3F(500, 500, 500));
   t3 = Platform::getRealMilliseconds();
   Vector<BVHProxy*> found = bvh.queryRegion(queryRegion);
   t4 = Platform::getRealMilliseconds();

   Con::printf("Queried region found %zu entities", found.size());
   if (found.size() == 0 || found.size() >= entityCount + globalCount)
      return false;

   // Move half the entities and update
   t5 = Platform::getRealMilliseconds();
   for (U32 i = 0; i < entityCount / 2; ++i)
   {
      Point3F min(500 + rng.randF() * 500, 500 + rng.randF() * 500, 500 + rng.randF() * 500);
      Point3F max = min + Point3F(1, 1, 1);
      proxies[i]->setBounds(Box3F(min, max));
      bvh.updateLeaf(nodes[i]);
   }
   t6 = Platform::getRealMilliseconds();

   // Query the new region
   Box3F queryRegion2(Point3F(500, 500, 500), Point3F(1000, 1000, 1000));
   U32 t7 = Platform::getRealMilliseconds();
   Vector<BVHProxy*> found2 = bvh.queryRegion(queryRegion2);
   U32 t8 = Platform::getRealMilliseconds();

   Con::printf("Queried region 2 found %zu entities", found2.size());
   if (found2.size() == 0 || found2.size() >= entityCount + globalCount)
      return false;

   // Clean up
   U32 t9 = Platform::getRealMilliseconds();
   for (U32 i = 0; i < entityCount + globalCount; ++i)
   {
      bvh.removeLeaf(nodes[i]);
      delete proxies[i];
   }
   U32 t10 = Platform::getRealMilliseconds();

   Con::printf("BVH 1000 entity + 100 global object test passed.");
   Con::printf("Performance metrics (ms):");
   Con::printf("  Insert regular: %u", t1 - t0);
   Con::printf("  Insert global:  %u", t2 - t1);
   Con::printf("  Query 1:        %u", t4 - t3);
   Con::printf("  Update 500:     %u", t6 - t5);
   Con::printf("  Query 2:        %u", t8 - t7);
   Con::printf("  Cleanup:        %u", t10 - t9);

   return true;
}

DefineEngineFunction(runBVHUnitTest, bool, (S32 entityCount, S32 globalCount), (1000,100),
   "Runs the BVH unit test with 1000 entities and 100 global objects, printing performance metrics.\n"
   "@return True if the test passes, false otherwise.")
{
   return runBVHUnitTest(entityCount, globalCount);
}
