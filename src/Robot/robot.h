	
#ifndef R_H
#define R_H

#include <ros/ros.h>
#include <iostream>
#include <string>
#include "myTuple.h"

using namespace std; 

enum robotState {idle,traveling,covering,done,dead};

class robot 
{
public:
	robot(int first, int second);
	void setPath(string path);
	void setState(robotState state);
	void setLocation(myTuple* newTuple);
	robotState getState();
	myTuple* getLocation();
	vector<myTuple*> getPath();
	bool isTheLast();
	void move();
private:
	myTuple* location;
	vector<myTuple*> path; 
	robotState state;

	vector<string> split(const string &s, char delim);
	int string2int(string s);
	
};


#endif