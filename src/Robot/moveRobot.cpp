#include "moveRobot.h"


moveRobot::moveRobot(int firstStart, int secondStart, int robot_id){
	canMove = false;
	ros::NodeHandle  nh;
	publisher  = nh.advertise<geometry_msgs::Twist>("/lizi_1/diff_driver/command", 10);	
	steps_pub = nh.advertise<ses::step>("steps", 10);
	path_sub = nh.subscribe("paths", 1, &moveRobot::pathCallback, this);
	laserSub = nh.subscribe("scan", 1, &moveRobot::scanCallback, this);
	me = new robot(firstStart, secondStart);
	robot_id = robot_id;
}

// Process the incoming laser scan message
void moveRobot::scanCallback(const sensor_msgs::LaserScan::ConstPtr& scan)
{
	bool isObstacleInFront = false;

    // Find the closest range between the defined minimum and maximum angles
    int minIndex = ceil((MIN_SCAN_ANGLE - scan->angle_min) / scan->angle_increment);
    int maxIndex = floor((MAX_SCAN_ANGLE - scan->angle_min) / scan->angle_increment);

    for (int currIndex = minIndex + 1; currIndex <= maxIndex; currIndex++) {
        if (scan->ranges[currIndex] < MIN_DIST_FROM_OBSTACLE) {
        	isObstacleInFront = true;
            break;
        }
    }

    if (isObstacleInFront) {
        ROS_INFO("Stop!");
        canMove = false;
    }
}
	
void moveRobot::pathCallback(const ses::Path::ConstPtr& path_msg){
	cout<<"robot catched a path!"<<endl;
	if(robot_id == path_msg->robot_id){
		me->setState((robotState)path_msg->state);
		me->setPath(path_msg->path);
		if(me->getState() == (robotState)dead){cout<<"moveRobot::robot "<<robot_id<<" died, so we became sad"<<endl;exit(0);}
		canMove = true;

	}
}


void moveRobot::publishStep(){
	
	ses::step step_msg;

	step_msg.robot_id = robot_id;
	step_msg.state = me->getState();
	step_msg.first_location = me->getLocation()->returnFirst();
	step_msg.second_location = me->getLocation()->returnSecond();
	step_msg.is_the_last = me->isTheLast();
	
	sleep(1.0);
	steps_pub.publish(step_msg);
	ros::spinOnce();
	
	ROS_INFO("Robot %d published on %d,%d step", robot_id, step_msg.first_location, step_msg.second_location);
}


void moveRobot::start(){
	publishStep();
	cout<<"first_location"<<endl;
	while (true) {
		ros::spinOnce();
		if(canMove){
			cout<<"start_moving_?"<<endl;
			vector<myTuple*> path = me->getPath();
			int size = path.size();

			if(size>1 && path[0]->returnFirst()== me->getLocation()->returnFirst() && path[0]->returnSecond()== me->getLocation()->returnSecond()){
		
				cout<<"location: "<<me->getLocation()->returnFirst()<<"."<<me->getLocation()->returnSecond()<<endl;
				me->move();
				cout<<"nlocation: "<<me->getLocation()->returnFirst()<<"."<<me->getLocation()->returnSecond()<<endl;
				moveToNext(path[0], path[1]);
				publishStep();
				ros::spinOnce();
			}else{
				canMove = false;
			}
		}
	}
}

 void moveRobot::step(Direction d){
 	getPose();
	double currentPlace,goal;
	int dir = 1;
	ros::Rate rate(40);
	if (d == UP || d == DOWN) {
		 currentPlace = currentLocationY;
		 if (d == DOWN) {dir = -1;}
		 goal = currentPlace + dir*DX;

		 while (ros::ok()) {
		 	if (abs(currentLocationY - goal) < placeTol) {break;}
		 	geometry_msgs::Twist msg;
		 	if (d == DOWN) {
		 		msg.linear.x = 0.2;
		 	} else {
		 		msg.linear.x = -0.2;
		 	}
		 	////debug code
		 	//cout << " need to dist " << goal << " current dist " << currentLocationY << endl;
		 	////debug code
		 	publisher.publish(msg);
		 	getPose();
		 }
	} else {
		 getPose();
		 if (d == LEFT) { dir = -1;}
		 goal = currentLocationX + dir*DXHorizontal;
		 while (ros::ok()) {
		 	if (abs(currentLocationX - goal) < placeTol) {break;}
		 	geometry_msgs::Twist msg;
		 	if (d == RIGHT) {
		 		msg.linear.x = 0.2;
		 	} else {
		 		msg.linear.x = -0.2;
		 	}
		 	////debug code
		 	//cout << " need to dist " << goal << " current dist " << currentLocationX << endl;
		 	////debug code
		 	publisher.publish(msg);		 	
		 	getPose();
		 }
	}
	for (int i = 0; i < 20; i++) {
		geometry_msgs::Twist stopCommand;
		stopCommand.angular.z = 0;
		publisher.publish(stopCommand);
	}
 }


void moveRobot::getPose() {
	tf::TransformListener listener;
	tf::StampedTransform transform;

	listener.waitForTransform("/map", "/lizi_1/base_link", ros::Time(0), ros::Duration(10.0) );

    try {
    	//listener.waitForTransform("/map", "/" + robotName + "/base_link", ros::Time(0), ros::Duration(60.0));
        listener.lookupTransform("/map",  "/lizi_1/base_link", ros::Time(0), transform);
        currentLocationX = transform.getOrigin().x();
        currentLocationY = transform.getOrigin().y();
        currentAngle = tf::getYaw(transform.getRotation());
    }
    catch (tf::TransformException & ex) {
        ROS_ERROR("%s", ex.what());
    } 
}


 void moveRobot::rotate(Direction d) {

	double directionAngles[] = { 0, M_PI / 2, M_PI, -M_PI/2 };
	double targetAngle = directionAngles[d];
	bool turnLeft;
	getPose();

	////debug code
	//cout << "current " << currentAngle << endl;
	////debug code

	// Decide to which side to rotate - left or right by choosing the small angle
	// bool turnLeft;
	if (targetAngle - currentAngle > 0 && targetAngle - currentAngle < M_PI)
		turnLeft = true;
	else if (targetAngle - currentAngle < -M_PI)
		turnLeft = true;
	else
		turnLeft = false;


	geometry_msgs::Twist rotateCommand;
	rotateCommand.angular.z = turnLeft ? angularSpeed : -angularSpeed;

	// How fast will we update the robot's movement
	ros::Rate rate(50);

	// Rotate until the robot reaches the target angle
	while (ros::ok() && abs(currentAngle - targetAngle) > angularTolerance * 50) {

		// The robot can reach the LEFT direction from negative PI or positive PI
		if (d == LEFT && (abs(currentAngle - (-M_PI)) <= angularTolerance * 50))
		    break;

		publisher.publish(rotateCommand);
		rate.sleep();
		getPose();
	}

	////debug code
	//cout << "Angle refinement #1" << endl;
	////debug code

	// Slow the speed near the target
	//rotateCommand.angular.z = turnLeft ? 0.1 * angularSpeed : -0.1 * angularSpeed;
	rotateCommand.angular.z = turnLeft ? 0.5 * angularSpeed : -0.5 * angularSpeed;

	while (ros::ok() && abs(currentAngle - targetAngle) > angularTolerance * 10) {
		// The robot can reach the LEFT direction from negative PI or positive PI
		if (d == LEFT && (abs(currentAngle - (-M_PI)) <= angularTolerance * 10))
			break;
			

		publisher.publish(rotateCommand);
		rate.sleep();
		getPose();

		
	}
	////debug code
	//cout << "Angle refinement #2" << endl;
	////debug code

	// Further refine the angle
	//rotateCommand.angular.z = turnLeft ? 0.05 * angularSpeed : -0.05 * angularSpeed;
	while (ros::ok() && abs(currentAngle - targetAngle) > angularTolerance) {
		// The robot can reach the LEFT direction from negative PI or positive PI
		if (d == LEFT && (abs(currentAngle - (-M_PI)) <= angularTolerance))
			break;
		

		publisher.publish(rotateCommand);
		rate.sleep();
		getPose();


	}
	////debug code
	//cout << "Angle refinement #3" << endl;
	////debug code


	for (int i = 0; i < 20; i++) {
		geometry_msgs::Twist stopCommand;
		stopCommand.angular.z = 0;
		publisher.publish(stopCommand);
	}
	
}


 void moveRobot::moveToNext(myTuple* location, myTuple* nextLocation){
 	Direction d;
 	if (location->returnFirst() == nextLocation->returnFirst() && location->returnSecond() > nextLocation->returnSecond()) {
 		d = LEFT;
	} else if (location->returnFirst() == nextLocation->returnFirst() && location->returnSecond() < nextLocation->returnSecond()) {
		d = RIGHT;
	} else if (location->returnFirst() > nextLocation->returnFirst() && location->returnSecond() == nextLocation->returnSecond()) {
		d = UP;	
	} else if (location->returnFirst() < nextLocation->returnFirst() && location->returnSecond() == nextLocation->returnSecond()) {
		d = DOWN;						
	}
	////debug code
	//cout << "DIR " << d <<endl;
	////debug code
	rotate(d);
	step(d);

 }