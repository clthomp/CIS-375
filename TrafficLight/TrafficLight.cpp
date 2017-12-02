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
	void setEndPoint(Intersection point); // located following Intersection class definition. Needs to be PUBLIC!
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
	Intersection(string name, vector<Road*>roads, vector<int>density) {		//TODO: This needs to call setEndPoint(*this) for all Roads in roadsThatIntersect per Sequence Diagram! --Dwight
		name = name;
		roadsThatIntersect = roads;
		roadDensity = density;

		findTogetherRoads();
	}

	void findTogetherRoads() {
		
	}
};

void Road::setEndPoint(Intersection point) {	//TODO: This needs to ensure endPoints <= 2! --Dwight
	endPoints.push_back(point);
}

class Map {
private:
	vector<Road> roads;
	//vector<Intersection> intersections; --Dwight

	int minimumLightTime;
	int maximumLightTime;
public:
	vector<Intersection> intersections;	//Should be private --Dwight
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

	//Requirements and Suggestions from Dwight

	void nuke() {	//Clears out all members of roads & intersections vectors. This will need to be called every time Input fails to parse! (When it throws)

		//TO BE IMPLEMENTED
		//
		//IMPORTANT:	NOTICED THAT PARAMETERS HAVE BEEN SEPERATED OUT FROM MAP CLASS IN LATEST COMMIT
		//				IF THIS WILL BE THE PLAN MOVING FORWARD, THEN NUKE() IS NOT NECESSARY, AS MAP CAN JUST BE
		//				DESTROYED AND RECREATED WITHOUT WORRY OF LOSING SET PARAMETERS!
	}

	Road * getroadPntr(string name, string id) {	//Returns pointer to matching road stored within roads vector, Nullptr if no match. So that Input can construct Intersections from roads within map!

		//TO BE IMPLEMENTED

		return nullptr;
	}

	bool roadExists(Road * road) { //So that Input creates valid Intersections. Could also be used to prevent duplicates!

		//TO BE IMPLEMENTED
		//ONLY COMPARE NAME AND ID

		return true;
	}

	/*
	bool intersectExists(Intersection * intersect) {	//Could be used to prevent Input from creating duplicates!

		//TO BE IMPLEMENTED
		//ONLY CAMPARE NAME
	}
	*/
};

class Input {	//NEEDS TO BE CONTAINED WITHIN A TRY BLOCK. WILL THROW STRINGS IF PARSING OR ANYTHING ELSE FAILS!
	
	typedef void(Map::*paramSet)(int);	//function pointer to Map method to set a specific parameter.

private:
	string path;			//path of .csv file (When file needs to be re-opened)
	ifstream input;			//input stream of .csv file
	Map* map;				//pointer to Map object to store Input into
	
	string roadname() {		//parse roadname non-terminal of input grammar; returns string of roadname

		char feed[40];	//name limit is 36 + 4 for nullsequence and cleanliness

		input.get(feed, 36, ',');

		return string(feed);
	}

	string roadid() {		//parse roadid non-terminal of input grammar; returns string of roadid

		char feed[10];	//id limit is 5 + 5 for nullsequence and cleanliness

		input.get(feed, 5, ',');

		return string(feed);
	}

	//IDENTICAL TO LANE() - WILL CONSIDER MERGING
	int traveltime() {	//parse traveltime non-terminal of input grammar; returns integer of traveltime

		char feed[15];	//int max is 10 digits + 5 for nullsequence and cleanliness

		input.get(feed, 10, ',');

		return stoi(feed, nullptr);
	}

	string intersectname() {	//parse intersectname non-terminal of input grammar; returns string of intersection name

		char feed[75];	//Intersection name limit is 72 characters + 3 for nullsequence and cleanliness

		input.get(feed, 72, ',');

		if (string(feed) == "END")	//A valid intersectname is indistinguishable from :END, so there must be an exception
			throw false;

		return string(feed);
	}
	/*
	float trafficdensity() {	//parse trafficdensity non-terminal of input grammar; returns integer of traffic density
	
	char feed[20];	//characters in FLT_MAX + 4 for nullsequence and cleanliness

	input.get(feed, 16, ',');

	return stof(feed, nullptr);
	*/
	int trafficdensity() {				//Temporary until merge
		char feed[15];					//
		input.get(feed, 10, ',');		//
		return stoi(feed, nullptr);		//
	}

	//IDENTICAL TO TRAVELTIME() - WILL CONSIDER MERGING
	int lane() {	//parse lane non-terminal of input grammar; returns integer of lane number

		char feed[15];	//int max is 10 digits + 5 for nullsequence and cleanliness

		input.get(feed, 10, '\n');

		return stoi(feed, nullptr);
	}

	bool road() {			//parses road non-terminal of input grammar; returns true if succeeds, false if not

		Road * blueprint;
		string name, id;
		int time;

		try {

			name = roadname();

			if (input.get() != ',')
				return false;

			id = roadid();

			if (input.get() != ',')
				return false;

			time = traveltime();

			if (input.get() != ',')
				return false;

			if (input.get() != '\n')
				return false;

			//PARSE IS SUCCESSFUL PAST THIS LINE: (IT IS OK TO THROW OUT OF FUNCTION!)

			//blueprint = new Road(name, id, time);
			blueprint = new Road(name, time);	//Temporary until merge occurs
			/*
			if (map->roadExists(blueprint))
				throw name + " " + id + " is duplicate!";
			*/
			map->importRoads(*blueprint);	//DOES NOT CURRENTLY CHECK IF ROAD ALREADY EXISTS

			delete blueprint;
		}
		/*
		catch (string s){	//If duplicate road is found.

			throw;
		}
		*/
		catch (...) {

			return false;
		}

		return true;
	}

	bool intersection() {	//parses intersection non-terminal of input grammar; returns true if succeeds, false if not

		Intersection * blueprint;
		string iname, rname, rid;
		Road * road;				//holds pointer to road already within Map: DO NOT DELETE!
		vector<Road*> roadlist;		//holds pointers to roads already within Map: DO NOT DELETE!
		int lanes;
		int density;				//Temporary until merge
		vector<int> densitylist;	//
		//float density;
		//vector<float> densitylist;
		bool go = true;
		streampos bookmark;

		try {

			if (input.get() != ':')
				return false;

			iname = intersectname();

			for (int i = 0; i < 3; i++)
				if (input.get() != ',')
					return false;

			if (input.get() != '\n')
				return false;

			do {

				bookmark = input.tellg();	//saves streampos of the current line
				
				try {

					rname = roadname();

					if (input.get() != ',')
						throw false;

					rid = roadid();

					if (input.get() != ',')
						throw false;

					density = trafficdensity();

					if (input.get() != ',')
						throw false;

					lanes = lane();

					if (input.get() != '\n')
						throw false;

					//PARSE IS SUCCESSFUL PAST THIS LINE: (IT IS OK TO THROW OUT OF FUNCTION!)

					road = map->getroadPntr(rname, rid);

					//if (road == nullptr)
					//	throw rname + " " + rid + " is not defined!";

					roadlist.push_back(road);
					densitylist.push_back(density);

				}
				catch (string s) {
					throw;
				}
				catch (...) {
					go = false;
				}

			} while (go);

			input.close();
			input.open(path);
			input.seekg(bookmark);	//restores saved streampos in case try block throws within the line

			blueprint = new Intersection(iname, roadlist, densitylist);

			/*
			if (map->intersectExists(blueprint))
				throw iname + " is duplicate!";
			*/
			map->importIntersections(*blueprint);	//DOES NOT CURRENTLY CHECK IF INTERSECTION ALREADY EXISTS

			delete blueprint;
		}
		catch (string s) {
			
			throw;
		}
		catch (...) {

			return false;
		}

		return true;
	}

	void roads() {			//parse roads non-terminal of input grammar

		char feed[10];
		bool go = true;
		streampos bookmark;

		input.getline(feed, 10);

		if (string(feed) != ":ROADS,,,")
			throw "Input file does not contain ':ROADS'!";

		do {

			bookmark = input.tellg();	//saves streampos of the current line

			go = road();

		} while (go);

		input.close();
		input.open(path);
		input.seekg(bookmark);	//restores saved streampos in case road() failed within the line
	}

	void intersections() {	//parse intersections non-terminal of input grammar

		char feed[20];
		bool go = true;
		streampos bookmark;

		input.getline(feed, 20);

		if (string(feed) != ":INTERSECTIONS,,,")
			throw "Input file does not contain ':INTERSECTIONS'!";
		
		do {

			bookmark = input.tellg();	//saves streampos of the current line

			go = intersection();

		} while (go);

		input.close();
		input.open(path);
		input.seekg(bookmark);	//restores saved streampos in case intersection() failed within the line
	}

	void data() {			//parse data non-terminal of input grammar

		roads();

		intersections();
	}
	
	void parseInput() {		//to parse input given from .csv file

			char feed[10];

			input.getline(feed, 10);

			if (string(feed) != ":BEGIN,,,")
				throw "Input file does not contain ':BEGIN'!";

			data();

			input.getline(feed, 10);

			if (string(feed) != ":END,,,")
				throw "Input file does not contain ':END'!";
	}

	paramSet pid(string & input) {		//returns function pointer of proper Map method given input
	
		if (input.substr(0, 11) == MIN_PID) {
			
			input = input.substr(11, input.size());
			
			return &Map::setMin;
		}
		else if (input.substr(0, 11) == MAX_PID) {

			input = input.substr(11, input.size());

			return &Map::setMax;
		}
		else {
			
			throw "Invalid Parameter Specifier!";
		}
	}

	int value(string & input) {		//returns int if input can be converted into one
	
		try {

			return stoi(input, nullptr);
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

		(map->*p)(value(input));
	}

public:
	Input(string inputPath, Map &m) {								//constructor that takes filepath of .csv file and Map object to store input into

		path = inputPath;

		input.exceptions(ifstream::failbit | ifstream::badbit);		//sets input to throw exceptions for logical or read errors
		input.open(inputPath);

		map = &m;
	}
	~Input()
	{
		if (input.is_open())
			input.close();
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

inline void strPopFront(string & input) {	//removes first character of 'input'
	
	input = input.substr(1, input.size());
}