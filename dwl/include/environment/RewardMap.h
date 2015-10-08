#ifndef DWL__ENVIRONMENT__REWARD_MAP__H
#define DWL__ENVIRONMENT__REWARD_MAP__H

#include <environment/SpaceDiscretization.h>
#include <environment/Feature.h>
#include <utils/utils.h>


namespace dwl
{

namespace environment
{

/**
 * @class RewardMap
 * @brief Abstract class for computing the reward map of the terrain
 */
class RewardMap
{
	public:
		/** @brief Constructor function */
		RewardMap();

		/** @brief Destructor function */
		virtual ~RewardMap();

		/** @brief Reset the reward map */
		void reset();

		/**
		 * @brief Adds a feature of the reward map
		 * @param Feature* the pointer of the feature to add
		 */
		void addFeature(Feature* feature);

		/**
		 * @brief Removes a feature of the reward map
		 * @param std::string the name of the feature to remove
		 */
		void removeFeature(std::string feature_name);

		/**
		 * @brief Abstract method for computing the reward map according the robot position and
		 * model of the terrain
		 * @param octomap::OcTree* The model of the environment
		 * @param const Eigen::Vector4d& The position of the robot and the yaw angle
		 */
		void compute(octomap::OcTree* model,
					 const Eigen::Vector4d& robot_state);

		/**
		 * @brief Computes the features and reward of the terrain given the octomap model and the key
		 * of the topmost cell of a certain position of the grid
		 * @param octomap::OcTree* Pointer to the octomap model of the environment
		 * @param const octomap::OcTreeKey& The key of the topmost cell of a certain position of
		 * the grid
		 */
		void computeRewards(octomap::OcTree* octomap,
							const octomap::OcTreeKey& heightmap_key);

		/**
		 * @brief Removes reward values outside the interest region
		 * @param const Eigen::Vector3d& State of the robot, i.e. 3D position and yaw orientation
		 */
		void removeRewardOutsideInterestRegion(const Eigen::Vector3d& robot_state);

		/**
		 * @brief Sets a interest region
		 * @param double Radius along the x-axis
		 * @param double Radius along the x-axis
		 */
		void setInterestRegion(double radius_x,
							   double radius_y);

		/**
		 * @brief Gets the properties of the cell
		 * @param RewardCell& Values of the cell
		 * @param double Reward value of the cell
		 * @param const Terrain& Information of the terrain in the specific cell
		 */
		void getCell(RewardCell& cell,
					 double reward,
					 const Terrain& terrain_info);

		/**
		 * @brief Gets the properties of the cell
		 * @param Key& Key of the cell
		 * @param const Eigen::Vector3d& Cartesian position of the cell
		 */
		void getCell(Key& key,
					 const Eigen::Vector3d& position);

		/**
		 * @brief Adds a cell to the reward map
		 * @param const RewardCell& Cell values for adding to the reward map
		 */
		void addCellToRewardMap(const RewardCell& cell);

		/**
		 * @brief Removes the cell to the reward map
		 * @param const Vertex& Cell vertex for removing to the reward map
		 */
		void removeCellToRewardMap(const Vertex& cell_vertex);

		/**
		 * @brief Adds a cell to the height map
		 * @param const Vertex& Cell vertex for adding to the height map
		 * @param double Height value
		 */
		void addCellToTerrainHeightMap(const Vertex& cell_vertex,
									   double height);

		/**
		 * @brief Removes a cell to the height map
		 * @param Vertex Cell vertex for removing to the height map
		 */
		void removeCellToTerrainHeightMap(const Vertex& cell_vertex);

		/**
		 * @brief Adds a new search area around the current position of the robot
		 * @param double Minimum Cartesian position along the x-axis
		 * @param double Maximum Cartesian position along the x-axis
		 * @param double Minimum Cartesian position along the y-axis
		 * @param double Maximum Cartesian position along the y-axis
		 * @param double Minimum Cartesian position along the z-axis
		 * @param double Maximum Cartesian position along the z-axis
		 * @param double Resolution of the grid
		 */
		void addSearchArea(double min_x, double max_x,
						   double min_y, double max_y,
						   double min_z, double max_z,
						   double grid_size);

		/**
		 * @brief Sets the neighboring area for computing physical properties of the terrain
		 * @param int Number of left neighbors
		 * @param int Number of right neighbors
		 * @param int Number of left neighbors
		 * @param int Number of right neighbors
		 * @param int Number of bottom neighbors
		 * @param int Number of top neighbors
		 */
		void setNeighboringArea(int back_neighbors, int front_neighbors,
								int left_neighbors, int right_neighbors,
								int bottom_neighbors, int top_neighbors);

		/**
		 * @brief Gets the environment resolution of the reward map
		 * @param bool Indicates if the key represents a plane or a height
		 * @return The resolution of the gridmap or height
		 */
		double getResolution(bool plane);

		/**
		 * @brief Sets the resolution of the environment discretization
		 * @param double Resolution of the environment
		 * @param bool Indicates if the key represents a plane or a height
		 */
		void setResolution(double resolution,
						   bool plane);

		/**
		 * @brief Gets the reward map
		 * @return The cell value per each vertex
		 */
		const std::map<Vertex,RewardCell>& getRewardMap() const;


	private:
		/** @brief Object of the SpaceDiscretization class for defining the grid routines */
		SpaceDiscretization space_discretization_;

		/** @brief Vector of pointers to the Feature class */
		std::vector<Feature*> features_;

		/** @brief Terrain reward values mapped using vertex id */
		std::map<Vertex,RewardCell> terrain_rewardmap_;

		/** @brief Terrain height map */
		std::map<Vertex,double> terrain_heightmap_;

		/** @brief Indicates if it was added a feature */
		bool is_added_feature_;

		/** @brief Indicates if it was added a search area */
		bool is_added_search_area_;

		/** @brief Interest area */
		double interest_radius_x_, interest_radius_y_;

		/** @brief Minimum height of the terrain */
		double min_height_;

		/** @brief Vector of search areas */
		std::vector<SearchArea> search_areas_;

		/** @brief Object of the NeighboringArea struct that defines the neighboring area */
		NeighboringArea neighboring_area_;

		/** @brief Indicates if it the first computation */
		bool is_first_computation_;

		/** @brief Defines if it is using the mean of the cloud */
		bool using_cloud_mean_;

		/** @brief Depth of the octomap */
		int depth_;
};

} //@namespace environment
} //@namespace dwl

#endif