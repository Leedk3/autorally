/*
* Software License Agreement (BSD License)
* Copyright (c) 2013, Georgia Institute of Technology
* All rights reserved.
*
* Redistribution and use in source and binary forms, with or without
* modification, are permitted provided that the following conditions are met:
*
* 1. Redistributions of source code must retain the above copyright notice, this
* list of conditions and the following disclaimer.
* 2. Redistributions in binary form must reproduce the above copyright notice,
* this list of conditions and the following disclaimer in the documentation
* and/or other materials provided with the distribution.
*
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
* AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
* IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
* DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE
* FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
* DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
* SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
* CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
* OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
* OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/
/**********************************************
 * @file autorally_pc_plant.cpp
 * @author Jake Sacks <jsacks@gatech.edu>
 * @date December 11, 2018
 * @copyright 2018 Georgia Institute of Technology
 * @brief Implementation of the AutorallyPCPlant class
 ***********************************************/
#include <autorally_control/path_integral/autorally_pc_plant.h>
#include <autorally_control/path_integral/autorally_plant.h>

#include <stdio.h>
#include <stdlib.h>

namespace autorally_control {

AutorallyPCPlant::AutorallyPCPlant(ros::NodeHandle global_node, ros::NodeHandle mppi_node,
                                   bool debug_mode, int hz, bool nodelet, int num_traj) : AutorallyPlant(
                                   global_node, mppi_node, debug_mode, hz, nodelet, num_traj)
{
  std::string pc_topic_name = getRosParam<std::string>("pc_topic", mppi_node);
  //Initialize the subscribers.
  points_sub_ = global_node.subscribe(pc_topic_name, 1, &AutorallyPCPlant::pointsCall, this);
  track_points_sub_ = global_node.subscribe("/stereo/track_points2", 1, &AutorallyPCPlant::trackPointsCall, this);
  obs_reset_sub_ = global_node.subscribe("obstacle_reset", 1, &AutorallyPCPlant::obsResetCall, this);
}

AutorallyPCPlant::~AutorallyPCPlant(){}

void AutorallyPCPlant::obsResetCall(autorally_msgs::resetObstacles obs_msg)
{
  reset_obstacles_ = obs_msg.reset;
  last_obs_reset_call_ = ros::Time::now();
}

void AutorallyPCPlant::pointsCall(sensor_msgs::PointCloud2Ptr points_msg)
{
  std::chrono::time_point<std::chrono::high_resolution_clock> t1 = std::chrono::high_resolution_clock::now();

  //Update the timestamp
  last_pc_call_ = ros::Time::now();

  //Copy pointer to point cloud
  points_ = points_msg;

  std::chrono::time_point<std::chrono::high_resolution_clock> t2 = std::chrono::high_resolution_clock::now();
  auto duration = std::chrono::duration_cast<std::chrono::milliseconds>( t2 - t1 ).count();
}

void AutorallyPCPlant::trackPointsCall(sensor_msgs::PointCloud2Ptr points_msg)
{
  //Update the timestamp
  last_track_pc_call_ = ros::Time::now();

  //Copy pointer to point cloud
  track_points_ = points_msg;
}

bool AutorallyPCPlant::getResetObstacles()
{
  return reset_obstacles_;
}

ros::Time AutorallyPCPlant::getLastPointCloudTime()
{
  return last_pc_call_;
}

ros::Time AutorallyPCPlant::getLastTrackPointCloudTime()
{
  return last_track_pc_call_;
}

ros::Time AutorallyPCPlant::getLastObstacleResetTime()
{
  return last_obs_reset_call_;
}

sensor_msgs::PointCloud2Ptr AutorallyPCPlant::getPointCloud()
{
  return points_;
}

sensor_msgs::PointCloud2Ptr AutorallyPCPlant::getTrackPointCloud()
{
  return track_points_;
}

void AutorallyPCPlant::dynRcfgCall(autorally_control::PathIntegralParamsConfig &config, int lvl) {
  boost::mutex::scoped_lock lock(access_guard_);
  costParams_.obstacle_coefficient = config.obstacle_coefficient;
  costParams_.obstacle_decay = config.obstacle_decay;
  costParams_.obstacle_buffer = config.obstacle_buffer;
  costParams_.obstacle_pad = config.obstacle_pad;
  lock.unlock();

  AutorallyPlant::dynRcfgCall(config, lvl);
}


void AutorallyPCPlant::pubCosts(float* nominal_costs, ros::Publisher cost_pub, int num_costs)
{
  cost_msg_.costs.clear();
  cost_msg_.header.stamp = ros::Time::now();
  for (int i = 0; i < num_costs; i++) {
    cost_msg_.costs.push_back(nominal_costs[i]);
  }
  cost_pub.publish(cost_msg_);
}

}