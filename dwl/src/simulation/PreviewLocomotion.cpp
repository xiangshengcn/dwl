#include <dwl/simulation/PreviewLocomotion.h>


namespace dwl
{

namespace simulation
{

PreviewLocomotion::PreviewLocomotion() : sample_time_(0.001), gravity_(9.81), mass_(0.)
{
	base_com_.setZero();
}


PreviewLocomotion::~PreviewLocomotion()
{

}


void PreviewLocomotion::resetFromURDFFile(std::string filename)
{
	// Reading the file
	std::ifstream model_file(filename.c_str());
	if (!model_file) {
		std::cerr << "Error opening file '" << filename << "'." << std::endl;
		abort();
	}

	// Reserving memory for the contents of the file
	std::string model_xml_string;
	model_file.seekg(0, std::ios::end);
	model_xml_string.reserve(model_file.tellg());
	model_file.seekg(0, std::ios::beg);
	model_xml_string.assign((std::istreambuf_iterator<char>(model_file)),
			std::istreambuf_iterator<char>());
	model_file.close();

	resetFromURDFFile(model_xml_string);
}


void PreviewLocomotion::resetFromURDFModel(std::string urdf_model)
{
	// Resetting the model of the floating-base system
	system_.resetFromURDFModel(urdf_model);

	// Initializing the dynamic from the URDF model
	dynamics_.modelFromURDFModel(urdf_model);

	// Setting the gravity magnitude from the rigid-body dynamic model
	gravity_ = system_.getRBDModel().gravity.norm();

	// Getting the total mass of the system
	mass_ = system_.getTotalMass();

	// Getting the floating-base CoM
	base_com_ = system_.getFloatingBaseCoM();
}


void PreviewLocomotion::setSampleTime(double sample_time)
{
	sample_time_ = sample_time;
}


void PreviewLocomotion::previewScheduled(PreviewTrajectory& trajectory,
										 const PreviewState& initial_state,
										 const std::vector<QuadrupedalPreviewParameters>& control_params)
{
//	unsigned int num_phases = control_params.size();
//	for (unsigned int i = 0; i < num_phases; i++) {
//		// Computing the number of samples
//		double phase_duration = control_params[i].four_support.duration;
//		unsigned int num_samples = round(phase_duration / sample_time_);
//
//		for (unsigned int k = 0; k < num_samples; k++) {
//			double current_time = sample_time_ * k;
//
//			// Computing the preview locomotion trajectory
//			WholeBodyState current_state;
//			stancePreview(current_state, current_time);
//
//			trajectory.push_back(current_state);
//		}
//
//	}





	// Initializing the foot pattern generator
//	double step_height = 0.1; //TODO set it
//	simulation::StepParameters step_params(params.duration, step_height);
//	Eigen::Vector3d initial_foot_pos;
//	Eigen::Vector3d target_foot_pos;
//	foot_pattern_generator_.setParameters(initial_state.time, initial_foot_pos, target_foot_pos, step_params);
}


void PreviewLocomotion::stancePreview(PreviewTrajectory& trajectory,
									  const PreviewState& state,
									  const StancePreviewParameters& params)
{
	// Computing the coefficients of the Spring Loaded Inverted Pendulum (SLIP) response
	double pendulum_height = 0.58; //TODO set it
	double slip_omega = sqrt(gravity_ / pendulum_height);
	double alpha = 2 * slip_omega * params.duration;
	Eigen::Vector2d slip_hor_proj = (state.com_pos - state.cop).head<2>();
	Eigen::Vector2d cop_disp = state.cop.head<2>() - params.terminal_cop;
	Eigen::Vector2d slip_hor_disp = state.com_vel.head<2>() * params.duration;
	Eigen::Vector2d beta_1 = slip_hor_proj / 2 + (slip_hor_disp - cop_disp) / alpha;
	Eigen::Vector2d beta_2 = slip_hor_proj / 2 - (slip_hor_disp - cop_disp) / alpha;

	// Computing the initial length of the pendulum
	double initial_length = (state.com_pos - state.cop).norm();

	// Computing the coefficients of the spring-mass system response
	double spring_gain = 60000.; //wn = 16.3//TODO set it
	double spring_omega = sqrt(spring_gain / mass_);
	double delta_length = params.terminal_length - initial_length;
	double d_1 = state.com_pos(rbd::Z) - initial_length + gravity_ /
			pow(spring_omega,2);
	double d_2 = state.com_vel(rbd::Z) / spring_omega -
			delta_length / (spring_omega * params.duration);

	// Computing the preview trajectory
	unsigned int num_samples = round(params.duration / sample_time_);
	for (unsigned int k = 0; k < num_samples; k++) {
		double time = sample_time_ * k;

		// Computing the current time of the preview trajectory
		PreviewState current_state;
		current_state.time = state.time + time;

		// Computing the horizontal motion of the CoM according to the SLIP system
		current_state.com_pos.head<2>() = beta_1 * exp(slip_omega * time) +
				beta_2 * exp(-slip_omega * time) +
				(cop_disp / params.duration) * time + state.cop.head<2>();
		current_state.com_vel.head<2>() = beta_1 * slip_omega * exp(slip_omega * time) -
				beta_2 * slip_omega * exp(-slip_omega * time) +
				cop_disp / params.duration;
		current_state.com_vel.head<2>() = beta_1 * pow(slip_omega,2) * exp(slip_omega * time) +
				beta_2 * pow(slip_omega,2) * exp(-slip_omega * time);

		// Computing the vertical motion of the CoM according to the spring-mass system
		current_state.com_pos(rbd::Z) = d_1 * cos(spring_omega * time) +
				d_2 * sin(spring_omega * time) + (delta_length / params.duration) * time +
				initial_length - gravity_ / pow(spring_omega,2);
		current_state.com_vel(rbd::Z) = -d_1 * spring_omega * sin(spring_omega * time) +
				d_2 * spring_omega * cos(spring_omega * time) +
				delta_length / params.duration;
		current_state.com_acc(rbd::Z) = -d_1 * pow(spring_omega,2) * cos(spring_omega * time) -
				d_2 * pow(spring_omega,2) * sin(spring_omega * time);

		// Computing the heading motion according to heading kinematic equation
		current_state.head_pos = state.head_pos + state.head_vel * time +
				0.5 * params.head_acc * pow(time,2);
		current_state.head_vel = state.head_vel + params.head_acc * time;
		current_state.head_acc = params.head_acc;

		// Appending the current state to the preview trajectory
		trajectory.push_back(current_state);
	}
}


void PreviewLocomotion::flightPreview(PreviewTrajectory& trajectory,
						   	   	   	  const PreviewState& state,
									  const FlightPreviewParameters& params)
{
	// Setting the gravity vector
	Eigen::Vector3d gravity_vec = Eigen::Vector3d::Zero();
	gravity_vec(rbd::Z) = -gravity_;

	// Computing the preview trajectory
	unsigned int num_samples = round(params.duration / sample_time_);
	for (unsigned int k = 0; k < num_samples; k++) {
		double time = sample_time_ * k;

		// Computing the current time of the preview trajectory
		PreviewState current_state;
		current_state.time = state.time + time;

		// Computing the CoM motion according to the projectile EoM
		current_state.com_pos = state.com_pos + state.com_vel * time +
				0.5 * gravity_vec * pow(time,2);
		current_state.com_vel = state.com_vel + gravity_vec * time;
		current_state.com_acc = gravity_vec;

		// Computing the heading motion by assuming that there isn't change in the angular momentum
		current_state.head_pos = state.head_pos + state.head_vel * time;
		current_state.head_vel = state.head_vel;
		current_state.head_acc = 0.;
	}
}


void PreviewLocomotion::toWholeBodyState(WholeBodyState& full_state,
										 const PreviewState& preview_state)
{
	// From the preview model we do not know the joint states, so we neglect the joint-related
	// components of the CoM
	rbd::linearPart(full_state.base_pos) = preview_state.com_pos - base_com_;
	rbd::linearPart(full_state.base_vel) = preview_state.com_vel;
	rbd::linearPart(full_state.base_acc) = preview_state.com_acc;

	full_state.base_pos(rbd::AZ) = preview_state.head_pos;
	full_state.base_vel(rbd::AZ) = preview_state.head_vel;
	full_state.base_acc(rbd::AZ) = preview_state.head_acc;

	//TODO Contact positions
}


void PreviewLocomotion::fromWholeBodyState(PreviewState& preview_state,
										   const WholeBodyState& full_state)
{
	// Computing the CoM position, velocity and acceleration
	preview_state.com_pos = system_.getSystemCoM(full_state.base_pos, full_state.joint_pos);
	preview_state.com_vel = system_.getSystemCoMRate(full_state.base_pos, full_state.joint_pos,
													 full_state.base_vel, full_state.joint_vel);
	preview_state.com_acc = full_state.base_acc.segment<3>(rbd::LX); //Neglecting the joint accelerations components
	preview_state.head_pos = full_state.base_pos(rbd::AZ);
	preview_state.head_vel = full_state.base_vel(rbd::AZ);
	preview_state.head_acc = full_state.base_acc(rbd::AZ);

	// Computing the CoP
	dynamics_.computeCenterOfPressure(preview_state.cop,
									  full_state.contact_eff,
									  full_state.contact_pos,
									  system_.getEndEffectorNames());

//	dynamics_.getActiveContacts(active_contacts,
//			  full_state.contact_eff,
//			  force_threshold); TODO
}

} //@namespace simulation
} //@namespace dwl
