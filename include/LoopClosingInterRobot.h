/**
* This file is part of ORB-SLAM2.
*
* Copyright (C) 2014-2016 Raúl Mur-Artal <raulmur at unizar dot es> (University of Zaragoza)
* For more information see <https://github.com/raulmur/ORB_SLAM2>
*
* ORB-SLAM2 is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* ORB-SLAM2 is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with ORB-SLAM2. If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef LOOPCLOSINGINTERROBOT_H
#define LOOPCLOSINGINTERROBOT_H

#include "KeyFrame.h"
#include "LocalMapping.h"
#include "Map.h"
#include "ORBVocabulary.h"
#include "Tracking.h"
#include "LoopClosing.h"

#include "KeyFrameDatabase.h"

#include <thread>
#include <mutex>
#include "Thirdparty/g2o/g2o/types/types_seven_dof_expmap.h"

// ROS
#include<ros/ros.h>
#include "std_msgs/String.h"

#include <cv_bridge/cv_bridge.h> // to convert desc to image
#include <sensor_msgs/image_encodings.h>

#include "distributed_mapper_msgs/Keyframe.h" // Keyframe
#include "distributed_mapper_msgs/Keypoint.h" // Keypoint
#include "distributed_mapper_msgs/Indices.h" // Indices
#include "distributed_mapper_msgs/Measurement.h" // Measurement


// Loop closure structure
typedef struct LoopClosureInterRobotStruct{
  gtsam::Key key1;
  gtsam::Key key2;
  cv::Mat mat; // relative pose of key2 in key1's frame
} LoopClosureInterRobot;


namespace ORB_SLAM2
{

class Tracking;
class LocalMapping;
class LoopClosing;
class KeyFrameDatabase;


class LoopClosingInterRobot
{
public:

    typedef pair<set<KeyFrame*>,int> ConsistentGroup;    
    typedef map<KeyFrame*,g2o::Sim3,std::less<KeyFrame*>,
        Eigen::aligned_allocator<std::pair<const KeyFrame*, g2o::Sim3> > > KeyFrameAndPose;

public:

    LoopClosingInterRobot(Map* pMap, KeyFrameDatabase* pDB, ORBVocabulary* pVoc,const bool bFixScale, int robotID = 0, char robotName = 'a');

    void SetTracker(Tracking* pTracker);

    void SetLocalMapper(LocalMapping* pLocalMapper);

    // Main function
    void Publish();
    void Subscribe(const distributed_mapper_msgs::Keyframe& keyframe);

    void InsertKeyFrame(KeyFrame *pKF);

    void RequestReset();

    // This function will run in a separate thread
    void RunGlobalBundleAdjustment(unsigned long nLoopKF);

    bool isRunningGBA(){
        unique_lock<std::mutex> lock(mMutexGBA);
        return mbRunningGBA;
    }
    bool isFinishedGBA(){
        unique_lock<std::mutex> lock(mMutexGBA);
        return mbFinishedGBA;
    }   

    void RequestFinish();

    bool isFinished();

    // added by @itzsid
    bool publishKeyFrame();
    bool loopClosureRetreived_;
    void setLoopClosureRetrievedToTrue();
    void setLoopClosureRetrievedToFalse();
    bool LoopClosureIsRetrieved();
    LoopClosureInterRobot loopClosure_;

    char robotName_;
    int robotID_;

    
    cv::Mat estimatedR_;
    cv::Mat estimatedT_;
    float estimatedS_;
    char matchedSymbol_;
    int matchedIndex_;
    
protected:

    bool CheckNewKeyFrames();

    bool DetectLoop(const DBoW2::BowVector& keyFrameBoWVec, int mnId,  float minScore);

    bool ComputeSim3(const vector<cv::Mat>& mapPoints,
                                              const vector<cv::KeyPoint>& keypoints,
                                              vector<int> indices,
                                              const vector<float>& mvLevelSigma2,
                                              const vector<float>& mvInvLevelSigma2,
                                            const cv::Mat& pose, const cv::Mat& K,
                                              const cv::Mat& descriptors,
                                              const DBoW2::FeatureVector& mFeatVec,
                                              int nrMapPoints, const vector<float> &maxDistanceInvariance,
                     const vector<float> &minDistanceInvariance, const vector<float> &mvScaleFactors,
                     const vector<cv::Mat> &pointDescriptors, float mnMinX, float mnMinY, float mnMaxX,
                     float mnMaxY, float mfGridElementWidthInv, float mfGridElementHeightInv, float mnGridRows,
                     float mnGridCols, int mnScaleLevels, float mvLogScaleFactor,
                     const std::vector<std::vector<std::vector<size_t> > >& mGrid,
                     float fx, float fy, float cx, float cy);

    void SearchAndFuse(const KeyFrameAndPose &CorrectedPosesMap);

    void CorrectLoop();

    void ResetIfRequested();
    bool mbResetRequested;
    std::mutex mMutexReset;

    bool CheckFinish();
    void SetFinish();
    bool mbFinishRequested;
    bool mbFinished;
    std::mutex mMutexFinish;

    Map* mpMap;
    Tracking* mpTracker;

    KeyFrameDatabase* mpKeyFrameDB;
    ORBVocabulary* mpORBVocabulary;

    LocalMapping *mpLocalMapper;

    std::list<KeyFrame*> mlpLoopKeyFrameQueue;

    std::mutex mMutexLoopQueue;

    // Loop detector parameters
    float mnCovisibilityConsistencyTh;

    // Loop detector variables
    KeyFrame* mpCurrentKF;
    KeyFrame* mpMatchedKF;
    std::vector<ConsistentGroup> mvConsistentGroups;
    std::vector<KeyFrame*> mvpEnoughConsistentCandidates;
    std::vector<KeyFrame*> mvpCurrentConnectedKFs;
    std::vector<MapPoint*> mvpCurrentMatchedPoints;
    std::vector<MapPoint*> mvpLoopMapPoints;
    cv::Mat mScw;
    cv::Mat mScm;
    g2o::Sim3 mg2oScw;
    gtsam::Key currentKey;
    gtsam::Key matchedKey;

    long unsigned int mLastLoopKFid;

    // Variables related to Global Bundle Adjustment
    bool mbRunningGBA;
    bool mbFinishedGBA;
    bool mbStopGBA;
    std::mutex mMutexGBA;
    std::thread* mpThreadGBA;

    // Fix scale in the stereo/RGB-D case
    bool mbFixScale;

    bool mnFullBAIdx;

    // Publishers and subscribers -- added by @itzsid
    ros::Publisher keyframe_pub_;
    ros::Subscriber keyframe_sub_;

    // Measurement publisher
    ros::Publisher measurement_pub_;

};

} //namespace ORB_SLAM

#endif // LoopClosingInterRobot_H
