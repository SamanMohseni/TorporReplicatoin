/*
*   Author: Seyed Saman Mohseni Sangtabi
*   Student number: 99210067
*   Low Power Design: Final Project
*/


#include <iostream>
#include <algorithm>
#include <fstream>
#include <vector>
#include <math.h>

using namespace std;


// Constants:
const double capacitance = 0.00033; // Farad
const double Vmin = 1.8; // Volt


// harvester output is emulated here: /////////////////////
// returns harvester's output power at time t
// t is givven in seconds and power returns in watts
double Ph(double t){
	// define harvested energy behavior here:
	int cc = t / 60.0;
	t -= cc*60.0;
	if (t >= 59){
		return 0.055; // 55mW
	}
	return 0.001035; // ~1mW
}
///////////////////////////////////////////////////////////


class Buffer {
public:
	Buffer(){
		Vbuff = 0; // initial value
	}

	void updateBuffer(double Ph, double consumedEnergy, double deltaT){
		double harvestedEnergy = Ph*deltaT;
		double deltaEnergy = harvestedEnergy - consumedEnergy;

		if (Vbuff > Vmin){
			totalHarvestedEnergy += consumedEnergy;
		}
		else{ // battery powered
			totalBatteryEnergy += consumedEnergy;
			deltaEnergy = harvestedEnergy;
		}

		Vbuff = sqrt(2.0*(0.5*capacitance*Vbuff*Vbuff + deltaEnergy) / capacitance);
		Vbuff = min(Vbuff, 5.2);

		timeElapsed += deltaT;
		Vlog.push_back(make_pair(timeElapsed, Vbuff));
	}

	double getVbuff(){
		return Vbuff;
	}

	double getTotalHarvestedEnergy(){
		return totalHarvestedEnergy;
	}

	double getTotalBatteryEnergy(){
		return totalBatteryEnergy;
	}

	vector<pair<double, double>> & getVlog(){
		return Vlog;
	}

private:
	double Vbuff;
	double totalHarvestedEnergy = 0;
	double totalBatteryEnergy = 0;
	double timeElapsed = 0;
	vector<pair<double, double>> Vlog;
};


class TorporLogic {
public:
	TorporLogic(){
		for (int i = 0; i < 8; i++){
			mask[i] = 0; // initially all tasks are unmasked.
		}
	}

	void config(int slotIndex, bool maskBit, double val){
		taskExecThres[slotIndex] = val;
		mask[slotIndex] = maskBit;
	}

	void config(int slotIndex, bool maskBit){
		mask[slotIndex] = maskBit;
	}

	int getNextTask(){
		return taskIndex;
	}

	void setPowerThres(double thres){
		powerThres = thres;
	}

	void update(Buffer &buffer, double Pht){
		taskIndex = -1;

		if (mask[0] && buffer.getVbuff() > taskExecThres[0]){
			taskIndex = 0;
			return;
		}
		if (Pht > powerThres && mask[0]){
			return;
		}
		for (int i = 7; i > 0; i--){
			if (mask[i] && buffer.getVbuff() > taskExecThres[i]){
				taskIndex = i;
				return;
			}
		}
	}

private:
	double taskExecThres[8];
	bool mask[8];
	int taskIndex = -1;
	double powerThres = 1 << 30;
};


struct Task{
	double duration;
	double power;
};

struct SoCLog{
	vector<int> taskExecCount;
	double timeElapsed;
	vector<pair<int, double>> taskExecLog; // <taskIndex, time>; possetive time indicates task start at time,
	// negative indicates task completion at -time
};

enum Policy { energy, power };

class SoC {
public:
	SoC(double _alwaysOnPower, vector<Task> _taskChain, TorporLogic &torporLogic, Policy _policy) : torporLogicRef(torporLogic){
		alwaysOnPower = _alwaysOnPower;
		taskChain = _taskChain;
		policy = _policy;
		taskInSlot.resize(taskChain.size());
		initTorpor();
		taskExecCount.resize(taskChain.size());
		for (int i = 0; i < taskExecCount.size(); i++){
			taskExecCount[i] = 0;
		}
		log.timeElapsed = 0;
	}

	SoCLog getLog(){
		log.taskExecCount = taskExecCount;
		return log;
	}

	// returns energy consumption during deltaT
	double update(double deltaT){
		log.timeElapsed += deltaT;

		if (runningTaskSlot == -1){ // no task is running
			runningTaskSlot = torporLogicRef.getNextTask();
			if (runningTaskSlot != -1){
				// new task started
				log.taskExecLog.push_back(make_pair(taskInSlot[runningTaskSlot], log.timeElapsed));
			}
			taskDuration = 0;

			return alwaysOnPower * deltaT;
		}
		else{
			taskDuration += deltaT;
			Task runningTask = taskChain[taskInSlot[runningTaskSlot]];

			if (taskDuration >= runningTask.duration){
				// execution complete
				taskExecCount[taskInSlot[runningTaskSlot]]++;
				log.taskExecLog.push_back(make_pair(taskInSlot[runningTaskSlot], -log.timeElapsed));

				// reconfigure TorporLogic
				for (int i = 1; i < taskChain.size(); i++){
					torporLogicRef.config(slotOfTask[i], (taskExecCount[i - 1] > taskExecCount[i]));
				}

				runningTaskSlot = -1;

				return (deltaT - (taskDuration - runningTask.duration)) * runningTask.power;
			}
			else{
				return deltaT * runningTask.power;
			}
		}
	}

private:

	double calculateVthres(Task t){
		double taskEnergy = t.power * t.duration;
		return sqrt(2.0*taskEnergy / capacitance + Vmin*Vmin);
	}

	void initTorpor(){
		vector<pair<double, int>> vec;
		switch (policy){
		case Policy::energy:
			taskInSlot[0] = taskChain.size() - 1;
			for (int i = 0; i < taskChain.size() - 1; i++){
				vec.push_back(make_pair(taskChain[i].power * taskChain[i].duration, i));
			}
			sort(vec.begin(), vec.end());

			for (int i = 0; i < vec.size(); i++){
				taskInSlot[i + 1] = vec[i].second;
			}
			break;
		case Policy::power:
			for (int i = 0; i < taskChain.size(); i++){
				vec.push_back(make_pair(taskChain[i].power, i));
			}
			sort(vec.begin(), vec.end());

			for (int i = 0; i < vec.size(); i++){
				taskInSlot[i] = vec[vec.size() - 1 - i].second;
			}
			break;
		default:
			break;
		}

		slotOfTask.resize(taskInSlot.size());
		for (int i = 0; i < taskChain.size(); i++){
			torporLogicRef.config(i, 0, calculateVthres(taskChain[taskInSlot[i]]));
			slotOfTask[taskInSlot[i]] = i;
		}

		torporLogicRef.config(slotOfTask[0], 1);
	}

private:
	double alwaysOnPower;
	vector<Task> taskChain;
	TorporLogic &torporLogicRef;
	Policy policy;
	vector<int> taskInSlot;
	vector<int> slotOfTask;
	vector<int> taskExecCount;
	int runningTaskSlot = -1; // initially no task is running.
	double taskDuration = 0;
	SoCLog log;
};


int main(){
	Buffer buffer;
	TorporLogic torpor;
	torpor.setPowerThres(0.01);

	const double alwaysOnPower = 0.0006;
	vector<Task> taskChain;
	Task sense, process, transmit;
	sense.duration = 0.035;
	sense.power = 0.077;
	process.duration = 0.333;
	process.power = 0.0051;
	transmit.duration = 0.088;
	transmit.power = 0.0051;

	taskChain.push_back(sense);
	taskChain.push_back(process);
	taskChain.push_back(transmit);

	// To change scheduling policy, swithch between 
	// Policy::power and Policy::energy as the last
	// parameter of this line: 
	SoC soc(alwaysOnPower, taskChain, torpor, Policy::power);

	const double deltaT = 0.001; // 1ms interval
	for (double t = deltaT; t < 600; t += deltaT){
		double energyConsumed = soc.update(deltaT);
		buffer.updateBuffer(Ph(t), energyConsumed, deltaT);
		torpor.update(buffer, Ph(t));
	}

	SoCLog log = soc.getLog();

	cout << "Executions/min: " << log.taskExecCount[2] / (log.timeElapsed / 60) << endl;
	cout << "Avarage harvested power: " << buffer.getTotalHarvestedEnergy() / log.timeElapsed * 1000.0 << endl;
	
	// write to file:
	ofstream sensefile, processfile, transmitfile, Vlog;
	sensefile.open("sensefile.txt");
	processfile.open("processfile.txt");
	transmitfile.open("transmitfile.txt");
	Vlog.open("vlog.txt");

	sensefile << 50 << "\t" << 0 << endl;
	processfile << 50 << "\t" << 0 << endl;
	transmitfile << 50 << "\t" << 0 << endl;

	for (int i = 0; i < log.taskExecLog.size(); i++){
		if (abs(log.taskExecLog[i].second) < 50){
			continue;
		}
		if (abs(log.taskExecLog[i].second) > 150){
			break;
		}
		switch (log.taskExecLog[i].first)
		{
		case 0:
			sensefile << abs(log.taskExecLog[i].second) << "\t" << !(log.taskExecLog[i].second > 0) << endl;
			sensefile << abs(log.taskExecLog[i].second) << "\t" << (log.taskExecLog[i].second > 0) << endl;
			break;
		case 1:
			processfile << abs(log.taskExecLog[i].second) << "\t" << !(log.taskExecLog[i].second > 0) << endl;
			processfile << abs(log.taskExecLog[i].second) << "\t" << (log.taskExecLog[i].second > 0) << endl;
			break;
		case 2:
			transmitfile << abs(log.taskExecLog[i].second) << "\t" << !(log.taskExecLog[i].second > 0) << endl;
			transmitfile << abs(log.taskExecLog[i].second) << "\t" << (log.taskExecLog[i].second > 0) << endl;
			break;
		default:
			cerr << "unexpected value" << endl;
			exit(1);
			break;
		}
	}

	sensefile << 150 << "\t" << 0 << endl;
	processfile << 150 << "\t" << 0 << endl;
	transmitfile << 150 << "\t" << 0 << endl;

	sensefile.close();
	processfile.close();
	transmitfile.close();

	for (int i = 0; i < buffer.getVlog().size(); i++){
		if (buffer.getVlog()[i].first < 50){
			continue;
		}
		if (buffer.getVlog()[i].first > 150){
			break;
		}
		Vlog << buffer.getVlog()[i].first << "\t" << buffer.getVlog()[i].second << endl;
	}
	
	return 0;
}