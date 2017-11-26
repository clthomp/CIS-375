#include <iostream>
#include <string>
#include <fstream> //for Input and Output
#include <vector>
#include <queue>

using namespace std;

void strPopFront(string & input);						//for Input & Output
const string PARAM_DELIM = "--";						//string to identify keyboard input as a parameter specifier
const string CALC_INIT = PARAM_DELIM + "calculate";		//string to initiate calculation in parser
const string MIN_PID = PARAM_DELIM + "mintiming";		//string for parser to identify request for min timing change
const string MAX_PID = PARAM_DELIM + "maxtiming";		//string for parser to identify request for max timing change

class Intersection;

class Road {
private:
	string name;
	int travelTime;
	vector<Intersection> endPoints;
	void setEndPoint(Intersection point); // located following Intersection class definition
public:
	Road(string name, int travelTime) {
		Road::name = name;
		Road::travelTime = travelTime;
	}
};

class Intersection {
private:
	// assume roads with the same name lead the same way
	//vector<Road*> roadsThatIntersect;
	string name;
	vector<int> roadDensity;
public:
	vector<Road*> roadsThatIntersect;
	struct TogetherRoad {
		vector<Road*> roads;
		int time;
	};
	vector<TogetherRoad*> togetherRoads;
	Intersection(string name, vector<Road*>roads, vector<int>density) {
		name = name;
		roadsThatIntersect = roads;
		roadDensity = density;

		findTogetherRoads();
	}

	void findTogetherRoads() {
		
	}
};

void Road::setEndPoint(Intersection point) {
	endPoints.push_back(point);
}


class Map {
private:
	vector<Road> roads;
	//vector<Intersection> intersections;

	int minimumLightTime;
	int maximumLightTime;
public:
	vector<Intersection> intersections;
	void importRoads(Road inputRoads) {
		roads.push_back(inputRoads);
	}

	void importIntersections(Intersection inputIntersections) {
		intersections.push_back(inputIntersections);
	}

	vector<vector<int>> lightOptimization() {
		struct Node {
			vector<Intersection> intersections;
			vector<vector<int>> lightTimes;
			int potentialFlow = 0;
			int currentIntersection = 0;

			Node(vector<Intersection> intersections, vector<vector<int>> lightTimes, int currentIntersection) {
				intersections = intersections;
				lightTimes = lightTimes;
				currentIntersection = currentIntersection;
				updatePotentialFlow();
			}

			void updatePotentialFlow() {
				potentialFlow = findPotentialFlow();
			}

			int findPotentialFlow() {
				// use traffic density to find sum of all cars passing through all intersections over time 't'
				int density = 0;
				for (int i = 0; i < currentIntersection; i++) {
					for (int j = 0; j < lightTimes[i].size(); j++) {
						density += lightTimes[i][j];
					}
				}

				// for lights not yet determined, use ideal flow
				// assume rest are in the ideal state
				// ideal be if 100% of cars were incoming from one way and 100% outgoing another way
				// as if 100% of cars incoming and outgoing the same 2 directions, with max light time on those, and min light time on the others
				for (int i = currentIntersection; i < intersections.size() ; i++) {
					for (int j = 0; j < lightTimes[i].size(); j++) {
						density += lightTimes[i][j];
					}
				}
				return density;
			}
		};
		struct LessThanByPromising {
			bool operator()(const Node& lhs, const Node& rhs) const
			{
				return lhs.potentialFlow < rhs.potentialFlow;
			}
		};

		// PQ of node potential timings
		priority_queue<Node, vector<Node>, LessThanByPromising> pq;

		vector<vector<int>> initialLightTimes;
		for (int i = 0; i < intersections.size(); i++) {
			vector<int> singleLightTime(intersections[i].roadsThatIntersect.size(), 0); /*TODO: div 2*/
			initialLightTimes.push_back(singleLightTime);
		}

		Node initial(intersections, initialLightTimes, 0);
		pq.push(initial);

		if (intersections.size() > 0) {
			while (!pq.empty()) {
				Node currentNode = pq.top();
				pq.pop();

				// add more nodes to pq
				// for each light at the current intersection, try adding 5 to it
				// TODO: multiple light changes in the same Node
				// right now each node only changes 1 light
				// FIX: each light has its own timing, needs to align timings for parallel roads

				// ensure haven't passed max number of intersections
				if (currentNode.currentIntersection < currentNode.lightTimes.size()) {
					int lightCount = currentNode.lightTimes[currentNode.currentIntersection].size();
					for (int i = 0; i < lightCount; i++) {
						for (int j = -5; j <= 5; j = j + 5) { // do for -5, 0, 5
							vector<vector<int>> newLightTimes = currentNode.lightTimes;
							newLightTimes[currentNode.currentIntersection][i] += j;
							Node newNode(intersections, newLightTimes, currentNode.currentIntersection + 1);
							pq.push(newNode);
						}
					}
				}

				// if the current node is a potential solution (reached last intersection)
				// and the top node is no more promising than the current Node,
				// then a solution has been found
				if (currentNode.currentIntersection == intersections.size() && pq.top().potentialFlow <= currentNode.potentialFlow) {
					// lightTimes belonging to the best node
					return currentNode.lightTimes;
				}
			}
		}

		return initial.lightTimes; 
	}

	void setMin(int min) {
		minimumLightTime = min;
	}

	void setMax(int max) {
		maximumLightTime = max;
	}
};

class Input {
	
	typedef void(*paramSet)(int);	//function pointer to Map method to set a specific parameter.

private:
	ifstream input;			//input stream of .csv file
	Map* map;				//pointer to Map object to store Input into
	
	string roadname() {		//parse roadname non-terminal of input grammar; returns string of roadname


	}

	bool road() {			//parses road non-terminal of input grammar; returns true if succeeds, false if not

		Road * blueprint;
		string name;
		int time;

		try {

			name = roadname();

			if (input.get() != ',')
				return false;

			time = traveltime();

			if (input.get() != ',')
				return false;

			if (input.get() != '\n')
				return false;

			blueprint = new Road(name, time);

			map->importRoads(*blueprint);
		}
		catch (...) {

			return false;
		}

		return true;
	}

	bool intersection() {	//parses intersection non-terminal of input grammar; returns true if succeeds, false if not

		Intersection * blueprint;
		string name;
		Road * road;
		vector<Road*> roadlist;
		int density, lanes;
		vector<int> densitylist;
		bool go;

		try {

			name = intersectname();

			if (input.get() != ',')
				return false;

			if (input.get() != '\n')
				return false;

			do {
				
				try {
					road = roadname();

					if (input.get() != ',')
						throw false;

					density = trafficdensity();

					if (input.get() != ',')
						throw false;

					lanes = lane();

					if (input.get() != '\n')
						throw false;


				}
				catch (...) {
					go = false;
				}

			} while (go);
		}
		catch (...) {

			return false;
		}

		return true;
	}

	void roads() {			//parse roads non-terminal of input grammar

		char feed[10];
		bool go = true;

		input.getline(feed, 10);

		if (feed != ":ROADS,,")
			throw "Input file does not contain ':ROADS'!";

		do {

			go = road();

		} while (go);
	}

	void intersections() {	//parse intersections non-terminal of input grammar

		char feed[20];
		bool go = true;

		input.getline(feed, 20);

		if (feed != ":INTERSECTIONS,,")
			throw "Input file does not contain ':INTERSECTIONS'!";

		do {

			go = intersection();

		} while (go);
	}

	void data() {			//parse data non-terminal of input grammar

		roads();

		intersections();
	}
	
	void parseInput() {		//to parse input given from .csv file

			char feed[10];

			input.getline(feed, 10);

			if (feed != ":BEGIN,,")
				throw "Input file does not contain ':BEGIN'!";

			data();

			input.getline(feed, 10);

			if (feed != ":END,,")
				throw "Input file does not contain ':END'!";
	}

	paramSet pid(string & input) {		//returns function pointer of proper Map method given input
	
		if (input.substr(0, 11) == MIN_PID) {
			
			input = input.substr(11, input.size());
			
			return map->setMin;
		}
		else if (input.substr(0, 11) == MAX_PID) {

			input = input.substr(11, input.size());

			return map->setMax;
		}
		else {
			
			throw "Invalid Parameter Specifier!";
		}
	}

	int value(string & input) {		//returns int if input can be converted into one
	
		try {

			return stoi(input);
		}
		catch (...) {

			throw "Invalid Integer!";
		}
	}

	void parseParam(string & input) {		//to parse input from keyboard during runtime

		paramSet p = pid(input);

		if (input[0] != ' ')
			throw "No space after Parameter Specifier!";
		strPopFront(input);

		p(value(input));
	}

public:
	Input(string inputPath, Map &m) {								//constructor that takes filepath of .csv file and Map object to store input into

		input.exceptions(ifstream::failbit | ifstream::badbit);		//sets input to throw exceptions for logical or read errors
		input.open(inputPath);

		map = &m;
	}

	void paramQuery(string input) {		//to begin parsing of parameter query 'input'

		if (input == CALC_INIT)
			parseInput();
		else
			parseParam(input);
	}
};

void main() {
	Map map;
	map.setMin(15);
	map.setMax(30);

	Road roadA1 = Road("A", 5);
	Road roadA2 = Road("A", 5);
	Road roadB1 = Road("B", 5);
	Road roadB2 = Road("B", 5);

	vector<Road*> roadList  = { &roadA1, &roadA2, &roadB1, &roadB2 };
	vector<int> densityList = { 1, 1, 1, 1 };

	Intersection intersectionAB = Intersection("A x B", roadList, densityList);

	map.intersections.push_back(intersectionAB);

	vector<vector<int>> lightTimings = map.lightOptimization();
	cout << "finished computation";
}

void strPopFront(string & input) {	//removes first character of 'input'
	
	input = input.substr(1, input.size());
}