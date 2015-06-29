#include <model/RobCoGenWholeBodyDynamics.h>


namespace dwl
{

namespace model
{

RobCoGenWholeBodyDynamics::RobCoGenWholeBodyDynamics() : kin_model_(NULL), initialized_kinematics_(false)
{

}


RobCoGenWholeBodyDynamics::~RobCoGenWholeBodyDynamics()
{

}


void RobCoGenWholeBodyDynamics::setKinematicModel(RobCoGenWholeBodyKinematics* kinematics)
{
	kin_model_ = kinematics;

	initialized_kinematics_ = true;
}


void RobCoGenWholeBodyDynamics::opAccelerationContributionFromJointVelocity(Eigen::VectorXd& jacd_qd,
																			const rbd::Vector6d& base_pos,
																			const Eigen::VectorXd& joint_pos,
																			const rbd::Vector6d& base_vel,
																			const Eigen::VectorXd& joint_vel,
																			enum rbd::Component component)
{
	if (!initialized_kinematics_)
		printf(RED "The kinematics model must be initialized " COLOR_RESET);

	// Computing the acceleration contribution from joint velocity for all end-effectors
	rbd::BodySelector body_set;
	for (rbd::BodyID::iterator body_iter = kin_model_->getBodyList().begin();
			body_iter != kin_model_->getBodyList().end();
			body_iter++)
	{
		std::string body_name = body_iter->first;
		body_set.push_back(body_name);
	}

	opAccelerationContributionFromJointVelocity(jacd_qd, base_pos, joint_pos, base_vel, joint_vel, body_set, component);
}


void RobCoGenWholeBodyDynamics::opAccelerationContributionFromJointVelocity(Eigen::VectorXd& jacd_qd,
																			const rbd::Vector6d& base_pos,
																			const Eigen::VectorXd& joint_pos,
																			const rbd::Vector6d& base_vel,
																			const Eigen::VectorXd& joint_vel,
																			rbd::BodySelector body_set,
																			enum rbd::Component component) // TODO Compute for other cases (Angular and full)
{
	if (!initialized_kinematics_)
		printf(RED "The kinematics model must be initialized " COLOR_RESET);

	// Computing the number of active end-effectors
	int num_body_set = 0;
	for (rbd::BodySelector::iterator body_iter = body_set.begin();
			body_iter != body_set.end();
			body_iter++)
	{
		std::string body_name = *body_iter;
		if (kin_model_->getBodyList().count(body_name) > 0)
			++num_body_set;
	}
	jacd_qd.resize(3 * num_body_set);
	jacd_qd.setZero();

	// Updating the dynamic and kinematic information
	propagateInverseDynamics(base_pos, joint_pos, base_vel, joint_vel,
							 rbd::Vector6d::Zero(), Eigen::VectorXd::Zero(joint_pos.size()));
	kin_model_->updateState(base_pos, joint_pos);
	updateState(base_pos, joint_pos);

	// Computing the acceleration contribution from joint velocity, i.e. J_d*q_d
	iit::rbd::VelocityVector body_vel;
	iit::rbd::VelocityVector body_acc;
	for (rbd::BodySelector::iterator body_iter = body_set.begin();
			body_iter != body_set.end();
			body_iter++)
	{
		int effector_counter = 0;
		std::string body_name = *body_iter;
		if (kin_model_->getBodyList().count(body_name) > 0) {
			body_vel = closest_link_motion_tf_.find(body_name)->second *
					closest_link_velocity_.find(body_name)->second;
			body_acc = closest_link_motion_tf_.find(body_name)->second *
								closest_link_acceleration_.find(body_name)->second;

			body_acc.segment(iit::rbd::LX, 3) = iit::rbd::linearPart(body_acc) +
					iit::rbd::angularPart(body_vel).cross(iit::rbd::linearPart(body_vel)); //TODO

			Eigen::Matrix4d homogeneous_tf = kin_model_->getHomogeneousTransform(body_name);
			jacd_qd.segment(effector_counter * 3, 3) = kin_model_->getBaseRotationMatrix().transpose()
					* iit::rbd::Utils::rotationMx(homogeneous_tf) * iit::rbd::linearPart(body_acc);

			++effector_counter;
		}
	}
}

} //@namespace model
} //@namespace dwl