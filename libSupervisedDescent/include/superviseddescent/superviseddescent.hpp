/*
 * superviseddescent.hpp
 *
 *  Created on: 14.11.2014
 *      Author: Patrik Huber
 */
#pragma once

#ifndef SUPERVISEDDESCENT_HPP_
#define SUPERVISEDDESCENT_HPP_

#include "regressors.hpp"
//#include "matserialisation.hpp"

#include "opencv2/core/core.hpp"

#ifdef WIN32
	#define BOOST_ALL_DYN_LINK	// Link against the dynamic boost lib. Seems to be necessary because we use /MD, i.e. link to the dynamic CRT.
	#define BOOST_ALL_NO_LIB	// Don't use the automatic library linking by boost with VS2010 (#pragma ...). Instead, we specify everything in cmake.
#endif
#include "boost/serialization/vector.hpp"

namespace superviseddescent {
	namespace v2 {

// or... LossFunction, CostFunction, Evaluator, ...?
// No inheritance, so the user can plug in anything without inheriting.
class TrivialEvaluationFunction
{

};

inline void noEval(const cv::Mat& currentPredictions) // do nothing
{
};

// template blabla.. requires Evaluator with function eval(Mat, blabla)
// or... Learner? SDMLearner? Just SDM? SupervDescOpt? SDMTraining?
// This at the moment handles the case of _known_ y (target) values.
// Requirements on RegressorType: See Regressor abstract class
// Requirements on UpdateStrategy: A function Mat(...Mat.. Regressor...);
//  for one sample. Also possibly (if a performance problem) an overload
//  for several samples at once.
template<class RegressorType, class UpdateStrategy=void>
class SupervisedDescentOptimiser
{
public:
	// We need a default constructor to load this from a boost serialisation archive
	SupervisedDescentOptimiser()
	{
	};

	SupervisedDescentOptimiser(std::vector<RegressorType> regressors) : regressors(regressors)
	{
	};


	// Hmm in case of LM / 3DMM, we might need to give more info than this? How to handle that?
	// Yea, some optimisers need access to the image data to optimise. Think about where this belongs.
	// The input to this will be something different (e.g. only images? a templated class?)
	// OnTrainingEpochCallback will get called
	// The callback functions signature should look like:
	//
	// $\mathbf{H}$ a function, should return a cv::Mat row-vector (1 row). Should process 1 training data. Optimally, whatever it is - float or a whole cv::Mat. But we can also simulate the 1D-case with a 1x1 cv::Mat for now.
	template<class H, class OnTrainingEpochCallback>
	void train(cv::Mat x, cv::Mat y, cv::Mat x0, H h, OnTrainingEpochCallback onTrainingEpochCallback)
	{
		// data = x = the parameters we want to learn, ground truth labels for them = y. x0 = c = initialisation
		// Simple experiments with sin etc.
		Mat currentX = x0;
		for (auto&& regressor : regressors) {
			// 1) Extract features where necessary
			Mat features; // We should rename this. In case of pose estimation, these are not features, but the projected 2D landmark coordinates (using the current parameter estimate (x)).
			for (int i = 0; i < currentX.rows; ++i) {
				features.push_back(h(currentX.row(i)));
			}
			Mat insideRegressor = features - y; // Todo: Find better name ;-) 'projection' or 'projectedParameters'?
			// We got $\sum\|x_*^i - x_k^i + R_k(h(x_k^i)-y^i)\|_2^2 $
			// That is: $-b + Ax = 0$, $Ax = b$, $x = A^-1 * b$
			// Thus: $b = x_k^i - x_*^i$. 
			// This is correct! It's the other way round than in the CVPR 2013 paper though. However,
			// it cancels out, as we later subtract the learned direction, while in the 2013 paper they add it.
			Mat b = currentX - x; // correct
			
			// 2) Learn using that data
			regressor.learn(insideRegressor, b);
			// 3) If not finished, 
			//    apply learned regressor, set data = newData or rather calculate new delta
			//    use it to learn the next regressor in next loop iter
			
			// If it's a performance problem, we should do R * featureMatrix only once, for all examples at the same time.
			// Note/Todo: Why does the old code have 'shapeStep = featureMatrix * R' the other way round?
			Mat x_k; // x_k = currentX - R * (h(currentX) - y):
			for (int i = 0; i < currentX.rows; ++i) {
				// No need to re-extract the features, we already did so in step 1)
				x_k.push_back(Mat(currentX.row(i) - regressor.predict(insideRegressor.row(i)))); // minus here is correct. CVPR paper got a '+'. See above for reason why.
			}
			currentX = x_k;
			onTrainingEpochCallback(currentX);
		}
	};

	template<class H>
	void train(cv::Mat x, cv::Mat y, cv::Mat x0, H h)
	{
		return train(x, y, x0, h, noEval);
	};

	// Returns the final prediction
	template<class H, class OnRegressorIterationCallback>
	cv::Mat test(cv::Mat y, cv::Mat x0, H h, OnRegressorIterationCallback onRegressorIterationCallback)
	{
		Mat currentX = x0;
		for (auto&& regressor : regressors) {
			Mat features;
			for (int i = 0; i < currentX.rows; ++i) {
				features.push_back(h(currentX.row(i)));
			}
			Mat insideRegressor = features - y; // Todo: Find better name ;-)
			Mat x_k; // (currentX.rows, currentX.cols, currentX.type())
			// For every training example:
			// This calculates x_k = currentX - R * (h(currentX) - y):
			for (int i = 0; i < currentX.rows; ++i) {
				Mat myInside = h(currentX.row(i)) - y.row(i);
				x_k.push_back(Mat(currentX.row(i) - regressor.predict(myInside))); // we need Mat() because the subtraction yields a (non-persistent) MatExpr
			}
			currentX = x_k;
			onRegressorIterationCallback(currentX);
		}
		return currentX; // Return the final predictions
	};

	template<class H>
	cv::Mat test(cv::Mat y, cv::Mat x0, H h)
	{
		return test(y, x0, h, noEval);
	};

	// Predicts the result value for a single example, using all regressors.
	// For the case where we have a template y (known y).
	template<class H>
	cv::Mat predict(cv::Mat x0, cv::Mat template_y, H h)
	{
		Mat currentX = x0;
		for (auto&& regressor : regressors) {
			// calculate x_k = currentX - R * (h(currentX) - y):
			cv::Mat myInside = h(currentX) - template_y;
			Mat x_k = currentX - regressor.predict(myInside);
			currentX = x_k;
		}
		return currentX;
	};

	friend class boost::serialization::access;
	template<class Archive>
	void serialize(Archive & ar, const unsigned int version)
	{
		ar & regressors;
	}

private:
	// Note/Todo:
	// Maybe we want a private function:
	// getNextX(currentX, h, y, regressor); to eliminate duplicate code in predict() and test().
	// Then, we could make an overload without y.
	// However, it wouldn't solve the problem of with/without y at the learning stage.

	// I think at some point we should separate the optimiser and the model.
	// But think about what depends on what, e.g. in the different cascade steps,
	// and for a future parallel RCRC, etc.
	std::vector<RegressorType> regressors;

	//UpdateStrategy updateStrategy;
};

	} /* namespace v2 */
} /* namespace superviseddescent */
#endif /* SUPERVISEDDESCENT_HPP_ */
