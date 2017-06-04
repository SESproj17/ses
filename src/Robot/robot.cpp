#include "robot.h"
#include <vector>
#include <sstream>
#include <cstdlib>
#include <string>

using namespace std;
void robot::setLocation(myTuple* newTuple) {
	this->location = newTuple;
}

vector<myTuple*> robot::getPath(){
	return this->path;
}
robot::robot(int first, int second){

	location = new myTuple(first,second);
	this->state = idle;

}

void robot::setPath(string path){

	vector<string> spath = split(path, ' ');
	this->path.resize(spath.size());
	for (int i = 0; i < spath.size(); ++i)
	{
		vector<string> loc = split(spath[i],',');
		this->path[i] = new myTuple(string2int(loc[0]),string2int(loc[1]));
	}
}

void robot::setProbs(string probs){

	vector<string> strProbs = split(probs, ' ');
	this->probs.resize(strProbs.size());
	for (int i = 0; i < strProbs.size(); ++i)
	{
		this->probs[i] = strtof((strProbs[i]).c_str(),0);
	}
}

void robot::setState(robotState state){
	this->state = state;
}

robotState robot::getState(){
	return this->state;
}
myTuple* robot::getLocation(){
	return this->location;
}
bool robot::isTheLast(){
	return this->path.size() == 1;
}

int robot::string2int(string s){
	int x;
	stringstream convert(s);
	convert>>x;
	return x;
}

vector<string> robot::split(const string &s, char delim) {
    stringstream ss(s);
    string item;
    vector<string> tokens;
    while (getline(ss, item, delim)) {
        tokens.push_back(item);
    }
    return tokens;
}

void robot::move(){
	this->probs.erase(this->probs.begin());
	this->path.erase(this->path.begin());
	this->location = path[0];
}
//flip coin and check if the robot that move on this cell died or not.
bool robot::imAlive() {
	srand(time(NULL));
	int x;float y;
	x = rand()%11;
	y = float(x)/10;
	if (y > probs[0]) {return true;}
	this->state = dead;
	return false;
}