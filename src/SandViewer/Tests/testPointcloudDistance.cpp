#include <vector>
#include <cassert>
#include <sstream>
#include <algorithm>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <nanoflann.hpp>

#include "Logger.h"
#include "ShaderPool.h"
#include "Ui/Window.h"
#include "Ui/TestGui.h"
#include "GlTexture.h"
#include "Filtering.h"
#include "ResourceManager.h"
#include "testSplineBlur.h"
#include "PointCloud.h"

#define MAKE_STR(streamline) (std::ostringstream() << streamline).str()

class PointCloudFlannAdaptor {
public:
	PointCloudFlannAdaptor(PointCloud & pointCloud) : m_pointCloud(pointCloud) {}

	inline size_t kdtree_get_point_count() const {
		return m_pointCloud.data().size() / m_pointCloud.frameCount();
	}
	
	inline float kdtree_get_pt(const size_t idx, const size_t dim) const {
		return m_pointCloud.data()[idx][static_cast<glm::length_t>(dim)];
	}

	template <class BBox> bool kdtree_get_bbox(BBox &bb) const { return false; }

private:
	PointCloud & m_pointCloud;
};

bool testPointcloudDistance()
{
	auto window = std::make_shared<Window>(512, 128, "SandViewer Tests - Point Cloud Distance");
	if (!window->isValid()) return EXIT_FAILURE;
	auto gui = std::make_unique<TestGui>(window);

	//---------------------------------------------
	gui->updateProgress(0.0f);
	gui->addMessage("Loading point cloud...");
	gui->render();
	window->swapBuffers();

	PointCloud pointCloud;
	pointCloud.loadBin(R"(G:\PhD\Data\SandViewer\beach-positions.bin)");

	//---------------------------------------------
	gui->addMessage("Building KD Tree...");
	gui->updateProgress(0.1f);

	typedef nanoflann::KDTreeSingleIndexAdaptor<
		nanoflann::L2_Simple_Adaptor<float, PointCloudFlannAdaptor>,
		PointCloudFlannAdaptor,
		3 /* dim */,
		size_t
	> KdTree;
	KdTree kdtree(3 /*dim*/, PointCloudFlannAdaptor(pointCloud), nanoflann::KDTreeSingleIndexAdaptorParams(10 /* max leaf */));
	kdtree.buildIndex();

	//---------------------------------------------
	gui->addMessage("Get point to point distances...");
	gui->updateProgress(0.2f);

	float minSqDistance = 999.9f;
	float maxSqDistance = 0.0f;
	float sumSqDistance = 0.0f;
	float sumDistance = 0.0f;

	int pointCount = kdtree.dataset.kdtree_get_point_count();
	int progressStep = pointCount / 1000;
	size_t count = 2;
	std::vector<size_t> indices(count);
	std::vector<float> sqDist(count);
	for (int i = 0 ; i < pointCount ; ++i)
	{
		count = kdtree.knnSearch(glm::value_ptr(pointCloud.data()[i]), count, &indices[0], &sqDist[0]);

		indices.resize(count);
		sqDist.resize(count);

		assert(count == 2);
		assert(indices[0] == i);

		// Gather statistics
		minSqDistance = std::min(minSqDistance, sqDist[1]);
		maxSqDistance = std::max(maxSqDistance, sqDist[1]);
		sumSqDistance += sqDist[1];
		sumDistance += glm::sqrt(sqDist[1]);

		if (i % progressStep == 0) {
			gui->updateProgress(glm::mix(0.2f, 0.6f, static_cast<float>(i + 1) / pointCount));
		}
	}

	float meanSqDistance = sumSqDistance / pointCount;
	float meanDistance = sumDistance / pointCount;
	float stdev = glm::sqrt(meanSqDistance - meanDistance * meanDistance);
	gui->addMessage("----------------------");
	gui->addMessage("Statistics:");
	gui->addMessage(MAKE_STR("  minDistance: " << glm::sqrt(minSqDistance)));
	gui->addMessage(MAKE_STR("  maxDistance: " << glm::sqrt(maxSqDistance)));
	gui->addMessage(MAKE_STR("  meanSqDistance: " << meanSqDistance));
	gui->addMessage(MAKE_STR("  meanDistance: " << meanDistance));
	gui->addMessage(MAKE_STR("  stdev: " << stdev));
	gui->addMessage("----------------------");

	//---------------------------------------------
	float cutoffDistance = meanDistance - 1.0f * stdev;
	float sqCutoffDistance = cutoffDistance * cutoffDistance;
	gui->addMessage(MAKE_STR("Removing all points at less than " << cutoffDistance << "..."));
	gui->updateProgress(0.6f);

	std::vector<bool> isDeleted(pointCount, false);
	std::vector<std::pair<size_t, float>> matches;
	nanoflann::SearchParams params;
	params.sorted = false;
	for (int i = 0; i < pointCount; ++i) {
		if (isDeleted[i]) continue;

		const size_t matchCount = kdtree.radiusSearch(glm::value_ptr(pointCloud.data()[i]), cutoffDistance, matches, params);
		LOG << "Found " << matchCount << " at a distance < " << cutoffDistance << " around point #" << i;
		for (int k = 0; k < matchCount; ++k) {
			int j = matches[k].first;
			if (j != i) {
				isDeleted[j] = true;
				assert(matches[k].second <= cutoffDistance);
				float d = glm::length(pointCloud.data()[i] - pointCloud.data()[j]);
				assert(matches[k].second == d);
			}
		}

		if (i % progressStep == 0) {
			gui->updateProgress(glm::mix(0.6f, 0.8f, static_cast<float>(i + 1) / pointCount));
		}
	}

	int deletedCount = 0;
	for (int i = 0; i < pointCount; ++i) {
		if (isDeleted[i]) ++deletedCount;
	}
	gui->addMessage(MAKE_STR("Deleted " << deletedCount << " points."));

	//---------------------------------------------
	gui->addMessage("Done.");
	gui->updateProgress(1.0f);
	
	window->swapBuffers();
	while (!window->shouldClose()) {
		window->pollEvents();
		gui->render();
		window->swapBuffers();
	}

	return true;
}

