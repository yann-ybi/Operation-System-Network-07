 /*-------------------------------------------------------------------------+
 |	Final Project CSC412 - Spring 2023										|
 |	A graphic front end for a box-pushing simulation.						|
 |																			|
 |	This application simply creates a glut window with a pane to display	|
 |	a colored grid and the other to display some state information.			|
 |																			|
 |	Current GUI:															|
 |		- 'ESC' --> exit the application									|
 |		- ',' --> slows down the simulation									|
 |		- '.' --> apeeds up  he simulation									|
 |																			|
 |	Created by Jean-Yves Hervé on 2018-12-05 (C version)					|
 |	Revised 2023-04-27														|
 +-------------------------------------------------------------------------*/ 
//
//  main.cpp
//  Final Project CSC412 - Spring 2023
// 
//  Created by Jean-Yves Hervé on 2018-12-05, Rev. 2023-12-01
//	This is public domain code.  By all means appropriate it and change is to your
//	heart's content.

// g++ -Wall -std=c++20 main.cpp gl_frontEnd.cpp -framework OpenGL -framework GLUT -o robot

#include <iostream>
#include <sstream>
#include <string>
#include <random>
#include <vector>
#include <map>
#include <cstdio>
#include <cstdlib>
#include <set>
#include <utility>
#include <fstream>
#include <unistd.h>
#include <thread>
#include <mutex>
#include <memory>
#include <chrono>
//
//
#include "glPlatform.h"
#include "typesAndConstants.h"
#include "gl_frontEnd.h"

using namespace std;

#if 0
//=================================================================
#pragma mark -
#pragma mark Function prototypes
//=================================================================
#endif

void displayGridPane(void);
void displayStatePane(void);
void printHorizontalBorder(ostringstream& outStream);
string printGrid(void);
void initializeApplication(void);
void cleanupAndQuit();
void generatePartitions();
void robotFunc(RobotInfo* robot);
void writeAction(int id, string action, char dir);
void closeOutputFile();

#if 0
//=================================================================
#pragma mark -
#pragma mark Application-level global variables
//=================================================================
#endif

//	Don't touch
extern int	gMainWindow, gSubwindow[2];

//-------------------------------------
//	Don't rename any of these variables
//-------------------------------------
//	The state grid's dimensions (arguments to the program)
int numRows;
int numCols;
int numBoxes;
int numDoors;	//	The number of doors.

int numLiveThreads = 0;		//	the number of live robot threads

//	robot sleep time between moves (in microseconds)
int robotSleepTime = 200000;

//	An array of C-string where you can store things you want displayed
//	in the state pane to display (for debugging purposes?)
//	Dont change the dimensions as this may break the front end
const int MAX_NUM_MESSAGES = 8;
const int MAX_LENGTH_MESSAGE = 32;
char** message;

//	Only absolutely needed if you tackle the partition EC
SquareType** grid;

int** robotLoc;
int** boxLoc;
int** doorLoc;

int* doorAssign;

ofstream outputFile;

vector<RobotInfo> robotInfoVec;
vector<std::thread> threads;

std::mutex fileMutex, numThreadMutex, robotVecMutex, moveMutex;

//	For extra credit section
random_device randDev;
default_random_engine engine(randDev());
vector<SlidingPartition> partitionList;

//	Change argument to 0.5 for equal probability of vertical and horizontal partitions
//	0 for only horizontal, and 1 for only vertical
bernoulli_distribution headsOrTails(1.0);

uniform_int_distribution<int> boundRow, boundCol, boxBoundRow, boxBoundCol, doorDist;

#if 0
//=================================================================
#pragma mark -
#pragma mark Function implementations
//=================================================================
#endif

//------------------------------------------------------------------------
//	You shouldn't have to change much in the main function besides
//	the initialization of numRows, numCos, numDoors, numBoxes.
//------------------------------------------------------------------------
int main(int argc, char** argv)
{
	if (argc != 5)
	{
		std::cerr << "Usage " << argv[0] << "<num_cols> <num_rows> <num_boxes> <num_doors>\n";
		return 1;
	}

	numRows = std::atoi(argv[1]);
    numCols = std::atoi(argv[2]);
	numBoxes = std::atoi(argv[3]);
    numDoors = std::atoi(argv[4]);

	if (numDoors < 1 || numDoors >= 10)
	{
		std::cerr << "The number of doors should be at least 1 but no more than 3\n";
	}

	//	Even though we extracted the relevant information from the argument
	//	list, I still need to pass argc and argv to the front-end init
	//	function because that function passes them to glutInit, the required call
	//	to the initialization of the glut library.
	initializeFrontEnd(argc, argv, displayGridPane, displayStatePane);

	//	Now we can do application-level initialization
	initializeApplication();

	string outStr = printGrid();
	cout << outStr << endl;
	
	//	Now we enter the main loop of the program and to a large extend
	//	"lose control" over its execution.  The callback functions that 
	//	we set up earlier will be called when the corresponding event
	//	occurs
	glutMainLoop();
	
	cleanupAndQuit();
		
	//	This will probably never be executed (the exit point will be in one of the
	//	call back functions).
	return 0;
}

/**
 * Set all the robots to dead
 */
void quit()
{
	for (auto& robot : robotInfoVec)
	{
		robot.isAlive = false;
	}
}

void cleanupAndQuit()
{
//	//	Free allocated resource before leaving (not absolutely needed, but
//	//	just nicer.  Also, if you crash there, you know something is wrong
//	//	in your code.
	for (auto& thread : threads)
		thread.join();

	for (int i=0; i< numRows; i++)
		delete []grid[i];
	delete []grid;
	for (int k=0; k<MAX_NUM_MESSAGES; k++)
		delete []message[k];
	delete []message;

	exit(0);
}

/**
 * Make doors at unique locations and add their coordinate to the doorLoc array
 */
void makeDoors() {

    doorLoc = new int*[numDoors];

    std::set<std::pair<int, int>> existingDoors;

	// Loop through the number of doors to generate their locations
    for (int k = 0; k < numDoors; k++) {
        doorLoc[k] = new int[2];
        int row, col;

		// Generate a unique location for the door
        do {
            row = boundRow(engine);
            col = boundCol(engine);
        } while (existingDoors.find({row, col}) != existingDoors.end());

        doorLoc[k][0] = row;
        doorLoc[k][1] = col;
        existingDoors.insert({row, col});
    }
}

/**
 * Make unique locations for robots, making sure they don't overlap with doors or boxes.
 */
void makeRobots() {
    robotLoc = new int*[numBoxes];

    std::set<std::pair<int, int>> existingLocations;

    // Add door and box locations to the existingLocations set
    for (int k = 0; k < numDoors; k++) {
        existingLocations.insert({doorLoc[k][0], doorLoc[k][1]});
    }
    for (int k = 0; k < numBoxes; k++) {
        existingLocations.insert({boxLoc[k][0], boxLoc[k][1]});
    }

    // Loop through the number of robots to generate their locations
    for (int k = 0; k < numBoxes; k++) {
        robotLoc[k] = new int[2];
        int row, col;

        // Generate a unique location for the robot
        do {
            row = boundRow(engine); 
            col = boundCol(engine); 
        } while (existingLocations.find({row, col}) != existingLocations.end());

        robotLoc[k][0] = row;
        robotLoc[k][1] = col;
        existingLocations.insert({row, col});
    }
}

/**
 * Make unique locations for boxes, making sure they don't overlap with doors.
 */
void makeBoxes() {

    boxLoc = new int*[numBoxes]; 

    std::set<std::pair<int, int>> existingLocations;

    // Add door locations to the existingLocations set
    for (int i = 0; i < numDoors; i++) {
        existingLocations.insert({doorLoc[i][0], doorLoc[i][1]});
    }

    // Loop through the number of boxes to generate their locations
    for (int k = 0; k < numBoxes; k++) {
        boxLoc[k] = new int[2]; 
        int row, col;

        // Generate a unique location for the box
        do {
            row = boxBoundRow(engine);
            col = boxBoundCol(engine);
        } while (existingLocations.find({row, col}) != existingLocations.end());

        boxLoc[k][0] = row; 
        boxLoc[k][1] = col;
        existingLocations.insert({row, col});
    }
}

/**
 * Assigns a door to each box/robot.
 */
void assignDoor() {
    doorAssign = new int[numBoxes];

    // Loop through the number of boxes/robots and assign a door to each
    for (int k = 0; k < numBoxes; k++)
	{
        doorAssign[k] = doorDist(engine);
	}
}

/**
 * Writes the locations of doors, boxes, and robots along with their destination doors to a file.
 */
void initializeOutputFile() {

	outputFile.open("robotSimulOut.txt");

	outputFile << "Grid - Width: " << numCols << " - Height: " << numRows << " - # of Doors: " << numDoors << ".\n";

	outputFile << "\n";

    // Write door locations to the file
    for (int k = 0; k < numDoors; k++) {
        outputFile << "D" << k << " @(" << doorLoc[k][0] << "," << doorLoc[k][1] << ")\n";
    }
    outputFile << "\n";

    // Write box locations to the file
    for (int k = 0; k < numBoxes; k++) {
        outputFile << "B" << k << " @(" << boxLoc[k][0] << "," << boxLoc[k][1] << ")\n";
    }
    outputFile << "\n";

    // Write robot locations and their destination doors to the file
    for (int k = 0; k < numBoxes; k++) {
        outputFile << "R" << k << " @(" << robotLoc[k][0] << "," << robotLoc[k][1] << ")" << " -> D" << doorAssign[k] << "\n";
    }
    outputFile << "\n";
}


/**
 * For each robot, a new RobotInfo struct is created and pushed into an array.
 * Its row and col pointers are set to the first and second elements of the current nested array, respectively. The doorRow and doorCol pointers are set to the first and second elements of the nested array at the index pointed to by the current element in doorAssign, respectively. 
 */
void initializeRobotInfos()
{
	for (int k = 0; k < numBoxes; k++)
	{
		RobotInfo robot;
		robot.id = k;
		robot.isAlive = true;

		robot.row = &robotLoc[k][0];
		robot.col = &robotLoc[k][1];

		robot.boxRow = &boxLoc[k][0];
		robot.boxCol = &boxLoc[k][1];

		robot.doorId = doorAssign[k];
		robot.doorRow = &doorLoc[doorAssign[k]][0];
		robot.doorCol = &doorLoc[doorAssign[k]][1];

		robot.isPushing = false;

		robot.moveCounter = 0;

		robotInfoVec.push_back(robot);
	}
}

void initializeApplication(void)
{
	//	Allocate the grid
	grid = new SquareType*[numRows];
	for (int i=0; i<numRows; i++)
		grid[i] = new SquareType [numCols];
	
	message = new char*[MAX_NUM_MESSAGES];
	for (int k=0; k<MAX_NUM_MESSAGES; k++)
		message[k] = new char[MAX_LENGTH_MESSAGE+1];
		
	//---------------------------------------------------------------
	//	This is the place where you should initialize the location
	//	of the doors, boxes, and robots, and create threads (not
	//	necessarily in that order).
	//---------------------------------------------------------------

	boundRow = uniform_int_distribution<int>(0, numRows - 1);
	boundCol = uniform_int_distribution<int>(0, numCols - 1);
	boxBoundRow = uniform_int_distribution<int>(1, numRows - 2);
	boxBoundCol = uniform_int_distribution<int>(1, numCols - 2);
	doorDist = uniform_int_distribution<int>(0, numDoors - 1);

	makeDoors();
	makeBoxes();
	makeRobots();
	assignDoor();
	
	initializeOutputFile();
	initializeRobotInfos();
	
	for (int k = 0; k < numBoxes; k++)
	{
		threads.emplace_back(robotFunc, &(robotInfoVec[k]));

		numThreadMutex.lock();
		numLiveThreads++;
		numThreadMutex.unlock();
	}
}

/**
 * Plans a robot's movement to push a box to a door.
 * @param robot A pointer to a RobotInfo object containing the robot's position, box position, and door position.
 */
void planTrip(RobotInfo* robot)
{
	int distC = *robot->boxCol - *robot->doorCol;
	int distR = *robot->boxRow - *robot->doorRow;

	int robotDistC = *robot->col - *robot->boxCol;
	int robotDistR = *robot->row - *robot->boxRow;

    auto addPlan = [&](char direction, int distance) {
        robot->plan.push_back(std::make_pair(direction, distance));
    };

	// box is on the East of the door
	if (distC > 0)
	{
		robot->boxPlan.push_back(std::make_pair('W', distC));

		if (robotDistC > 1)
			addPlan('W', robotDistC - 1);

		else if (robotDistC <= 0)
			addPlan('E', abs(robotDistC) + 1);

		if (robotDistR > 0)
			addPlan('N', robotDistR);

		else if (robotDistR < 0)
			addPlan('S', abs(robotDistR));

	}
	// box is on the West of the door
	else if (distC < 0)
	{
		robot->boxPlan.push_back(std::make_pair('E', abs(distC)));

		if (robotDistC >= 0)
			addPlan('W', robotDistC + 1);

		else if (robotDistC < -1)
			addPlan('E', abs(robotDistC) - 1);

		if (robotDistR > 0)
			addPlan('N', robotDistR);

		else if (robotDistR < 0)
			addPlan('S', abs(robotDistR));
	}
	// box is on the same column as the door
	else
	{
		int newRobotCol = *robot->boxCol;
		int newDistC = abs(newRobotCol - *robot->col);

		if (robotDistC > 0)
			addPlan('W', newDistC);

		else if (robotDistC < 0)
			addPlan('E', newDistC);

		int newRobotRow = 0;
		if (distR > 0)
			newRobotRow = *robot->boxRow + 1;

		else if (distR < 0)
			newRobotRow = *robot->boxRow - 1;
			
		int newDistR = *robot->row - newRobotRow;

		if (newDistR < 0)
			addPlan('S', newDistR);
		else if (newDistR > 0)
			addPlan('N', abs(newDistR));
	}

	// box is on the East or West of the door
	if (!robot->boxPlan.empty()) 
	{
		addPlan(robot->boxPlan.front().first, robot->boxPlan.front().second);
		robot->pushIndex = robot->plan.size() - 1;
	
		if (distR > 0)
		{
			robot->boxPlan.push_back(std::make_pair('N', distR));
			addPlan('S', 1);
			
			addPlan(robot->boxPlan.front().first, 1);
			//
			addPlan(robot->boxPlan.back().first, robot->boxPlan.back().second);
			//
		}
		else if (distR < 0)
		{
			robot->boxPlan.push_back(std::make_pair('S', abs(distR)));

			addPlan('N', 1);
			addPlan(robot->boxPlan.front().first, 1);
			//
			robot->plan.push_back(robot->boxPlan.back());
			//
		}
	}
	// box is on the the same column as the door
	else
	{
		if (distR > 0)
		{
			robot->boxPlan.push_back(std::make_pair('N', distR));
			addPlan('N', distR);
			robot->pushIndex = robot->plan.size() - 1;
		}
		else if (distR < 0)
		{
			robot->boxPlan.push_back(std::make_pair('S', abs(distR)));
			addPlan('S', abs(distR));
			robot->pushIndex = robot->plan.size() - 1;
		}
	}
}

bool isLocationFree(RobotInfo* robot, int row, int col)
{
    // Iterate through robotLoc to check if any robot occupies the given location
    for (int i = 0; i < numBoxes; i++)
    {
        if (robotLoc[i][0] == row && robotLoc[i][1] == col)
        {
            return false; // The location is occupied by a robot
        }
    }

    // Iterate through boxLoc to check if any box occupies the given location
    for (int i = 0; i < numBoxes; i++)
    {
        if (boxLoc[i][0] == row && boxLoc[i][1] == col && robot->id != i)
        {
            return false; // The location is occupied by a box
        }
    }

    return true; // The location is free
}

/**
 * Moves the robot in the specified direction for a given distance.
 * @param id The identifier of the robot.
 * @param dir The direction to move the robot ('N', 'S', 'W', 'E').
 * @param dist The distance to move the robot.
 * @param isPushing Whether the robot is pushing a box while moving.
 */
void moveRobot(RobotInfo* robot, char dir, int dist)
{
    // Offsets for each direction
    int rowOffset = 0;
    int colOffset = 0;

    switch (dir)
    {
    case 'N':
        rowOffset = -1;
        break;
    case 'S':
        rowOffset = 1;
        break;
    case 'W':
        colOffset = -1;
        break;
    case 'E':
        colOffset = 1;
        break;
    default:
        return;
    }

    std::string action = robot->isPushing ? "push" : "move";

    // Move the robot and update its position
    for (int k = 0; k < dist; k++)
    {
        if (robot->isPushing)
        {
			while (true)
			{
				if (!robot->isAlive) return;
			
				moveMutex.lock();
				
				if (isLocationFree(robot, boxLoc[robot->id][0] + rowOffset, boxLoc[robot->id][1] + colOffset))
				{
					boxLoc[robot->id][0] += rowOffset;
					boxLoc[robot->id][1] += colOffset;

					robotLoc[robot->id][0] += rowOffset;
					robotLoc[robot->id][1] += colOffset;

					moveMutex.unlock();

					writeAction(robot->id, action, dir);
					break;
				}
				else moveMutex.unlock();
			}
        }
        else
        {
			while (true)
			{
				if (!robot->isAlive) return;

				moveMutex.lock();

				if (isLocationFree(robot, robotLoc[robot->id][0] + rowOffset, robotLoc[robot->id][1] + colOffset))
				{
					robotLoc[robot->id][0] += rowOffset;
					robotLoc[robot->id][1] += colOffset;

					moveMutex.unlock();

					writeAction(robot->id, action, dir);
					break;
				}
				else moveMutex.unlock();
			}
        }

        usleep(robotSleepTime);
    }
}

/**
 * Checks if the robot is currently pushing
 */
void checkIfRobotIsPushing(RobotInfo* robot)
{
	robot->moveCounter++;

	robot->isPushing = (robot->moveCounter == robot->pushIndex) || (robot->moveCounter == robot->plan.size() - 1);
}

/**
 * Write action performed by the robot on the output file
 */
void writeAction(int id, string action, char dir)
{
	fileMutex.lock();
	outputFile << "robot " << id << " " << action << " " << dir << "\n";
	fileMutex.unlock();
}

/**
 * Write end command to the output file
 */
void writeEndCmd(int id)
{
	fileMutex.lock();
	outputFile << "robot " << id << "  end" << "\n";
	fileMutex.unlock();
}

/**
 * Get the lock to close the file
 */
void closeOutputFile()
{
	fileMutex.lock();
	outputFile.close();
	fileMutex.unlock();
}

/**
 * check if all threads are dead
 */
void checkForClosure()
{
	numThreadMutex.lock();
	numLiveThreads--;
	if (numLiveThreads <= 0)
	{
		closeOutputFile();
	}
	numThreadMutex.unlock();
}

/**
 * main function for the robot to perform tasks
 */
void robotFunc(RobotInfo* robot)
{
    // Plan the trip by generating a list of robot commands (move/push)
    planTrip(robot);

    // Continue executing moves while the robot is alive
    while (robot->isAlive)
    {
        // Execute one move from the plan
        for (const auto& [dir, dist] : robot->plan)
        {
            moveRobot(robot, dir, dist);
            checkIfRobotIsPushing(robot);
			if (!robot->isAlive)
				break;
        }

        // Set isAlive to false to exit the loop after executing all moves
		writeEndCmd(robot->id);
        robot->isAlive = false;

		robotLoc[robot->id][0] = -1;
		robotLoc[robot->id][1] = -1;

		boxLoc[robot->id][0] = -1;
		boxLoc[robot->id][1] = -1;
    }

	checkForClosure();
}

#if 0
//=================================================================
#pragma mark -
#pragma mark You probably don't need to look/edit below
//=================================================================
#endif


//	Rather that writing a function that prints out only to the terminal
//	and then
//		a. restricts me to a terminal-bound app;
//		b. forces me to duplicate the code if I also want to output
//			my grid to a file,
//	I have two options for a "does it all" function:
//		1. Use the stream class inheritance structure (the terminal is
//			an iostream, an output file is an ofstream, etc.)
//		2. Produce an output file and let the caller decide what they
//			want to do with it.
//	I said that I didn't want this course to do too much OOP (and, to be honest,
//	I have never looked seriously at the "stream" class hierarchy), so we will
//	go for the second solution.
string printGrid(void)
{
	//	some ugly hard-coded stuff
	static string doorStr[] = {"D0", "D1", "D2", "D3", "DD4", "D5", "D6", "D7", "D8", "D9"};
	static string robotStr[] = {"r0", "r1", "r2", "r3", "r4", "r5", "r6", "r7", "r8", "r9"};
	static string boxStr[] = {"b0", "b1", "b2", "b3", "b4", "b5", "b6", "b7", "b8", "b9"};
	
	if (numDoors > 10 || numBoxes > 10)
	{
		cout << "This function only works for small numbers of doors and robots" << endl;
		exit(1);
	}

	//	I use sparse storage for my grid
	map<int, map<int, string> > strGrid;
	
	//	addd doors
	for (int k=0; k<numDoors; k++)
	{
		strGrid[doorLoc[k][0]][doorLoc[k][1]] = doorStr[k];
	}
	//	add boxes
	for (int k=0; k<numBoxes; k++)
	{
		strGrid[boxLoc[k][0]][boxLoc[k][1]] = boxStr[k];
		strGrid[robotLoc[k][0]][robotLoc[k][1]] = robotStr[k];
	}
	
	ostringstream outStream;

	//	print top border
	printHorizontalBorder(outStream);
	
	for (int i=0; i<numRows; i++)
	{
		outStream << "|";
		for (int j=0; j<numCols; j++)
		{
			if (strGrid[i][j].length() > 0)
				outStream << " " << strGrid[i][j];
			else {
				outStream << " . ";
			}
		}
		outStream << "|" << endl;
	}
	//	print bottom border
	printHorizontalBorder(outStream);

	strGrid.clear();
	return outStream.str();
}

void printHorizontalBorder(ostringstream& outStream)
{
	outStream << "+--";
	for (int j=1; j<numCols; j++)
	{
		outStream << "---";
	}
	outStream << "-+" << endl;
}

void generatePartitions(void)
{
	const unsigned int NUM_PARTS = (numCols+numRows)/4;

	//	I decide that a partition length  cannot be less than 3  and not more than
	//	1/4 the grid dimension in its Direction
	const unsigned int MIN_PARTITION_LENGTH = 3;
	const unsigned int MAX_HORIZ_PART_LENGTH = numCols / 4;
	const unsigned int MAX_VERT_PART_LENGTH = numRows / 4;
	const unsigned int MAX_NUM_TRIES = 20;
	uniform_int_distribution<unsigned int> horizPartLengthDist(MIN_PARTITION_LENGTH, MAX_HORIZ_PART_LENGTH);
	uniform_int_distribution<unsigned int> vertPartLengthDist(MIN_PARTITION_LENGTH, MAX_VERT_PART_LENGTH);
	uniform_int_distribution<unsigned int> boundRow(1, numRows-2);
	uniform_int_distribution<unsigned int> boundCol(1, numCols-2);

	for (unsigned int w=0; w< NUM_PARTS; w++)
	{
		//	Case of a vertical partition
		if (headsOrTails(engine))
		{
			bool goodPart = false;

			//	I try a few times before giving up
			for (unsigned int k=0; k<MAX_NUM_TRIES && !goodPart; k++)
			{
				//	let's be hopeful
				goodPart = true;

				//	select a column index
				unsigned int col = boundCol(engine);
				unsigned int length = vertPartLengthDist(engine);

				//	now a random start row
				unsigned int startRow = 1 + boundRow(engine) % (numRows-length-1);
				for (unsigned int row=startRow, i=0; i<length && goodPart; i++, row++)
				{
					if (grid[row][col] != SquareType::FREE_SQUARE)
						goodPart = false;
				}

				//	if the partition is possible,
				if (goodPart)
				{
					//	add it to the grid and to the partition list
					SlidingPartition part;
					part.isVertical = true;
					for (unsigned int row=startRow, i=0; i<length && goodPart; i++, row++)
					{
						grid[row][col] = SquareType::VERTICAL_PARTITION;
						GridPosition pos = {row, col};
						part.blockList.push_back(pos);
					}

					partitionList.push_back(part);
				}
			}
		}
		// case of a horizontal partition
		else
		{
			bool goodPart = false;

			//	I try a few times before giving up
			for (unsigned int k=0; k<MAX_NUM_TRIES && !goodPart; k++)
			{
				//	let's be hopeful
				goodPart = true;

				//	select a row index
				unsigned int row = boundRow(engine);
				unsigned int length = vertPartLengthDist(engine);

				//	now a random start row
				unsigned int startCol = 1 + boundCol(engine) % (numCols-length-1);
				for (unsigned int col=startCol, i=0; i<length && goodPart; i++, col++)
				{
					if (grid[row][col] != SquareType::FREE_SQUARE)
						goodPart = false;
				}

				//	if the wall first, add it to the grid and build SlidingPartition object
				if (goodPart)
				{
					SlidingPartition part;
					part.isVertical = false;
					for (unsigned int col=startCol, i=0; i<length && goodPart; i++, col++)
					{
						grid[row][col] = SquareType::HORIZONTAL_PARTITION;
						GridPosition pos = {row, col};
						part.blockList.push_back(pos);
					}

					partitionList.push_back(part);
				}
			}
		}
	}
}
