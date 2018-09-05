#include "ros/ros.h"
#include <geometry_msgs/WrenchStamped.h>
#include <dynamixel_workbench_msgs/JointCommand.h>
// #include <dynamixel_workbench_msgs/DynamixelStateList.h>
#include <std_msgs/String.h>
#include <sensor_msgs/JointState.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <phidget22.h>
#include "PhidgetHelperFunctions.h"
#include <cmath>
#include <math.h>
#include <fstream>  // for file streaming

using namespace::std;
std::ofstream dataFile;

float Fx_a, Fy_a = 0;float Fx, Fy = 0;
float pot1, pot2 = 0;
float sep1, sep2 = 0;
float pot10 = round(0.06*0.4433*10000)/10000;// zero position of pot1
float pot20 = round(0.06*0.4716*10000)/10000;// zero position of pot2
float dyn1_0 = -(150*M_PI)/180 + (110)*(300*M_PI)/(1024*180); // zero separation between magnets, where 0-300 deg of dynamixel is encoded to 0-1023 bits
float dyn2_0 = -(150*M_PI)/180 + (370)*(300*M_PI)/(1024*180); // JointState.position returns values between -(150/180*M_PI) and (150/180*M_PI)
float c1    = 28.4104; //magnet constants
float c2    = 206.3563;
//ros::Duration Ts  = ros::Duration(0.01); // 100 Hz

float magnetForceModel(float pot0, float pot, float sep){
  float x;
  x   = pot - pot0; // disp from center position
  return 2*c1*exp(-c2*sep)*sinh(c2*x);
}
/* ------------Phidget function files------------*/
/*Phidget attach handler*/
static void CCONV onAttachHandler(PhidgetHandle ph, void *ctx) {
  printf("\tSetting DataInterval to 1ms\n");
	PhidgetVoltageRatioInput_setDataInterval((PhidgetVoltageRatioInputHandle)ph, 1);
	printf("\tSetting VoltageRatio ChangeTrigger to 0.0\n");
	PhidgetVoltageRatioInput_setVoltageRatioChangeTrigger((PhidgetVoltageRatioInputHandle)ph, 0.0);
}

/* Triggered on voltage change*/
static void CCONV onVoltageRatioChangeHandler1(PhidgetVoltageRatioInputHandle ph, void *ctx, double voltageRatio) {
  // unable to get channel information here, therefore, hardcoding
  //ROS_INFO("[pot1] -> VoltageRatio: %.4f\n", voltageRatio);
  //ROS_INFO("[pot10]: %.4f, [pot1]: %.4f\n, [sep1]: %.4f\n", pot10, pot1, sep1);

  pot1 = round(voltageRatio*0.06*10000)/10000;
  Fx   = magnetForceModel(pot10,pot1,sep1);
  ROS_INFO("Fx: [%f], Fx actual: [%f]", Fx,Fx_a); // x-axis of force sensor and mechanism are aligned in opposite direction
}
static void CCONV onVoltageRatioChangeHandler2(PhidgetVoltageRatioInputHandle ph, void *ctx, double voltageRatio) {
  // unable to get channel information here, therefore, hardcoding
  //ROS_INFO("[pot2] -> VoltageRatio: %.4f\n", voltageRatio);
  //ROS_INFO("[pot20]: %.4f, [pot2]: %.4f\n, [sep2]: %.4f\n", pot20, pot2, sep2);

  pot2 = round(voltageRatio*0.06*10000)/10000;
  Fy   = magnetForceModel(pot20,pot2,sep2);
  ROS_INFO("Fy: [%f], Fy actual: [%f]", Fy,Fy_a);
}
/* Phidget start function */
void phidgetStart(int channel, int phidgetStartStatus){
  PhidgetVoltageRatioInputHandle ch= NULL;
  ChannelInfo channelInfo;
  channelInfo.deviceSerialNumber = 324343;
  channelInfo.channel            = channel;
  if (phidgetStartStatus == 1){
    PhidgetVoltageRatioInput_create(&ch);// Allocate a new Phidget channel
    Phidget_setDeviceSerialNumber((PhidgetHandle)ch,  channelInfo.deviceSerialNumber);  	// Set device serial number
    Phidget_setChannel((PhidgetHandle)ch, channelInfo.channel);
    Phidget_setOnAttachHandler((PhidgetHandle)ch, onAttachHandler, NULL);
    if (channelInfo.channel == 0){
      PhidgetVoltageRatioInput_setOnVoltageRatioChangeHandler(ch, onVoltageRatioChangeHandler1, NULL);
    }else if(channelInfo.channel == 1){
      PhidgetVoltageRatioInput_setOnVoltageRatioChangeHandler(ch, onVoltageRatioChangeHandler2, NULL);
    }
    /* Open the channel */
    printf("Opening and Waiting for Attachment...\n");
    Phidget_open((PhidgetHandle) ch);
  }
 else {
    //clear phidget handlers
   	printf("Cleaning up pot %i ...\n", channelInfo.channel);
   	Phidget_close((PhidgetHandle)ch);
   	PhidgetVoltageRatioInput_delete(&ch);
 }
}
/*-------------Force sensor callback--------------*/
void forceCallback(const geometry_msgs::WrenchStamped str)
{
  Fx_a  = str.wrench.force.x;
  Fy_a  = str.wrench.force.y;
}
/*---------------dynamixel callback --------------*/
void dynCallback(const sensor_msgs::JointState pos)
{
  float dyn1 = pos.position[0];
  float dyn2 = pos.position[1];
  sep1 = std::abs((.02546*(dyn1 - dyn1_0))/2); // r*theta, here r = 25.46/2 mm
  sep2 = std::abs((.02546*(dyn2 - dyn2_0))/2);
}


/* -------Main file------*/
int main(int argc, char **argv)
{
  int result, numSensors;
  const char *err;
  int phidgetStartStatus = 1; // 1 for true and zero for false
  int channel1 = 0;
  int channel2 = 1;
  dataFile.open("/home/srisadha/ros/tremor_mech/data/force.csv");
  dataFile << "Time" << "," <<"Fx" <<"," << "Fy" <<"," << "Fx_actual" <<"," << "Fy_actual" << "," <<"Sep1"  << "," << "Sep2"<< "," << "disp_x"<< "," << "disp_y"<< endl;

  // Initialization
  ros::init(argc, argv, "sensor_data");
  ros::NodeHandle nh;
  // Subscribe to the topics
  ros::Subscriber sub1 = nh.subscribe("wrench",1,forceCallback);
  ros::Subscriber sub2 = nh.subscribe("joint_states",1,dynCallback);

  phidgetStart(channel1, phidgetStartStatus);
  phidgetStart(channel2, phidgetStartStatus);

  ros::Rate loop_rate(250);
  while(ros::ok()){
    dataFile <<ros::Time::now()<<"," <<Fx <<"," << Fy <<"," << Fx_a<<"," << Fy_a  << "," << sep1 << "," << sep2 << "," << pot1-pot10<< "," << pot2-pot20<<endl;
    ros::spinOnce();
    loop_rate.sleep();
  }
  phidgetStartStatus = 0; // close the phidget now
  phidgetStart(channel1, phidgetStartStatus);
  phidgetStart(channel2, phidgetStartStatus);

  printf("\nExiting...\n");
  dataFile.close(); // close file
  return 0;
}



// Ros service client --> this method is not working
// ros::ServiceClient client = nh.serviceClient<dynamixel_workbench_msgs::JointCommand>("/joint_sri");
// dynamixel_workbench_msgs::JointCommand dyn;
// dyn.request.unit = "raw";
// dyn.request.id   = 1;
// dyn.request.goal_position = 400;
// client.call(dyn);
// dyn.response.result;
