/*

// Try this input
CenterLineImport-reduced.csv
--iterations 3
--change 5
--calculate

runtime-test.csv
--iterations 1
--calculate
*/

#include <iostream>
#include <string>
#include <fstream> //for Input and Output
#include <vector>
#include <queue>
#include <string>
#include <algorithm>
#include <time.h>

// somewhat issue: density "leaves" the system depending on the use test case design
// if there are roads with no intersection on 1 side at the edges or elsewhere

using namespace std;

// allows printing debug messages
bool DEBUG_MODE = false;

// all parameter values should be located here
class Settings {
public:
	int minimumLightTime = 15;
	int maximumLightTime = 45;
	// should be between min and max with regards to being divisible by lightTimeTestValueChange
	int defaultLightValue = 30;

	// time it takes before cars begin moving after a light turns green
	float carStartupTime = 3;
	// average number of cars that can pass through a light per 1 second after cars have sped up
	float carPassingRate = 1.0;
	// how many cars per 1 unit of density
	float densityToCarRatio = 1.0;
	// the importance travel time has on shifting density. Inverse of time passed per iteration
	float travelTimeDensityMultiplier = 10;

	// how much to try moving each light time up or down at each iteration
	int lightTimeTestValueChange = 1;

	// how many times to run optimization
	// more iterations will give more accurate results, but will take longer to compute
	// each individual iteration should take the same amount of time
	int iterations = 1;

	void setMin(float value) {
		minimumLightTime = max(float(1), value);
		if (value <= 0) { cout << "Warning: Minimum Light Timing must be positive." << endl;}
	}
	void setMax(float value) {
		maximumLightTime = max(float(1), value);
		if (value <= 0) { cout << "Warning: Maximum Light Timing must be positive." << endl; }
	}
	void setDefaultLightValue(float value) {
		defaultLightValue = max(float(1), value);
		if (value <= 0) { cout << "Warning: Default Light Timing must be positive." << endl; }
	}
	void setCarStartupTime(float value) { carStartupTime = value; }
	void setCarPassingRate(float value) { carPassingRate = value; }
	void setDensityToCarRatio(float value) { densityToCarRatio = value; }
	void setTravelTimeDensityMultiplier(float value) { travelTimeDensityMultiplier = value; }
	void setLightTimeTestValueChange(float value) { lightTimeTestValueChange = value; }
	void setIterations(float value) { iterations = value; }
};
Settings SETTINGS;

void strPopFront(string & input);						//for Input & Output
const string PARAM_DELIM = "--";						//string to identify keyboard input as a parameter specifier
const string CALC_INIT = PARAM_DELIM + "calculate";		//string to initiate calculation in parser
const string MIN_PID = PARAM_DELIM + "mintiming";		//string for parser to identify request for min timing change
const string MAX_PID = PARAM_DELIM + "maxtiming";		//string for parser to identify request for max timing change
const string DEFAULT_PID = PARAM_DELIM + "default";
const string STARTUP_PID = PARAM_DELIM + "startup";
const string PASSING_PID = PARAM_DELIM + "passing";
const string DENSITY_PID = PARAM_DELIM + "density";
const string TRAVEL_PID = PARAM_DELIM + "travel";
const string CHANGE_PID = PARAM_DELIM + "change";
const string ITERATIONS_PID = PARAM_DELIM + "iterations";
const int PARAM_NUM = 9;									//the amount of user-controlled parameters; defined above

class Intersection;

// class written by Robert Piepsney
class Road {
private:

public:
	string name;
	string id;
	float travelTime;
	vector<Intersection> endPoints;
	void setEndPoint(Intersection point); // located following Intersection class definition

	Road(string name, string id, float travelTime) {
		this->name = name;
		this->id = id;
		this->travelTime = travelTime;
	}
};

// class written by Robert Piepsney
class Intersection {
private:
	// unite all roads of the same name
	void findTogetherRoads() {
		for (int i = 0; i < roadsThatIntersect.size(); i++) {
			bool isFound = false;
			for (int j = 0; j < togetherRoads.size(); j++) {
				// pair up TogetherRoad that exist with the same road name
				if (roadsThatIntersect[i]->name == togetherRoads[j].name) {
					isFound = true;
					(togetherRoads[j].roads).push_back(roadsThatIntersect[i]);
					(togetherRoads[j].density).push_back(roadDensity[i]);
				}
			}

			// if an existing one is not found, create a new one
			if (!isFound) {
				togetherRoads.push_back(TogetherRoad(roadsThatIntersect[i], roadDensity[i]));
			}
		}
	}
public:
	vector<Road*> roadsThatIntersect;
	string name;
	vector<int> laneList;
	vector<float> roadDensity; // parallel to roadsThatIntersect

	// used to store which 2 roads have the same name
	// ASSUMPTION: and therefore their lights are at the same time
	struct TogetherRoad {
		vector<Road*> roads;
		vector<float> density;
		string name;
		TogetherRoad(Road* initialRoad, float density) {
			roads.push_back(initialRoad);
			this->density.push_back(density);
			this->name = initialRoad->name;
		}
	};
	vector<TogetherRoad> togetherRoads;

	Intersection(string name, vector<Road*>roads, vector<int> laneList, vector<float>density) {
		this->name = name;
		roadsThatIntersect = roads;
		this->laneList = laneList;
		roadDensity = density;

		for (int i = 0; i < roadsThatIntersect.size(); i++) {
			roadsThatIntersect[i]->setEndPoint(*this);
		}

		findTogetherRoads();
	}

	// if roadIndex is an index for a Road in roadsThatIntersect
	// return the index of a TogetherRoad which contains the same Road
	int roadIndexToLight(int roadIndex) {
		for (int i = 0; i < togetherRoads.size(); i++) {
			for (int j = 0; j < togetherRoads[i].roads.size(); j++) {
				if (togetherRoads[i].roads[j] == roadsThatIntersect[roadIndex]) {
					return i;
				}
			}
		}

		if (DEBUG_MODE) {
			cout << "Error: No road to light index found" << endl;
		}

		return 0;
	}
};

void Road::setEndPoint(Intersection point) {	//TODO: This needs to ensure endPoints <= 2! --Dwight
	endPoints.push_back(point);
}

// Find cars that can pass through a light over 't' time
float getCarsPassing(float t) {
	// subtract time it takes cars to get up to reasonable speed
	t -= SETTINGS.carStartupTime;
	// after reaching reasonable speed,
	// multiply by how many cars can pass through the light per second
	t *= SETTINGS.carPassingRate;
	return max(float(0.0), t);
}

// class written by Robert Piepsney
class Map {
private:
	vector<Road> roads;
public:
	vector<Intersection> intersections;	//Should be private --Dwight
	void importRoads(Road inputRoads) {
		roads.push_back(inputRoads);
	}
	void importIntersections(Intersection inputIntersections) {
		intersections.push_back(inputIntersections);
	}

	vector<vector<int>> tempSolutions;

	// recursive method to generate all possible changes for light times
	// a light time can either increase, decrease, or stay the same by the value given from SETTINGS
	// adds all possible sets to tempSolutions
	void getLightChanges(vector<int> currentChanges, int current) {
		if (current >= currentChanges.size()) {
			tempSolutions.push_back(currentChanges);
		}
		else {
			getLightChanges(currentChanges, current + 1);

			currentChanges[current] += SETTINGS.lightTimeTestValueChange;
			getLightChanges(currentChanges, current + 1);

			currentChanges[current] -= SETTINGS.lightTimeTestValueChange * 2;
			getLightChanges(currentChanges, current + 1);
		}
	}

	// spread density between intersections based on an estimated value
	// of how many cars move in each direction,
	// based on the solution light times at the end of the iteration
	void doSpreadDensity(vector<vector<int>> currentLightTimes) {
		// minus density must be bounded so that it doesn't go below 0
		// and add to density based on the same restriction

		// first, calculate the bounded minus for all intersections
		// then add based on that
		vector<vector<float>> minusDensities;
		for (int i = 0; i < intersections.size(); i++) {
			vector<float> densityPerIntersection(intersections[i].roadsThatIntersect.size(), 0.0);
			minusDensities.push_back(densityPerIntersection);
		}

		for (int i = 0; i < intersections.size(); i++) {
			for (int j = 0; j < intersections[i].roadDensity.size(); j++) {
				Road* thisRoad = intersections[i].roadsThatIntersect[j];
				int lightIndex = intersections[i].roadIndexToLight(j);
				int lightTime = currentLightTimes[i][lightIndex];

				// outgoing traffic
				// expected density removed
				minusDensities[i][j] = getCarsPassing(lightTime) * SETTINGS.densityToCarRatio *
					float(intersections[i].laneList[j]) /
					(thisRoad->travelTime * SETTINGS.travelTimeDensityMultiplier);

				minusDensities[i][j] = min(intersections[i].roadDensity[j], minusDensities[i][j]);
			}
		}

		// for all the intersections, change density based on cars coming from other intersections of incoming roads
		for (int i = 0; i < intersections.size(); i++) {
			for (int j = 0; j < intersections[i].roadDensity.size(); j++) {
				// for the density of each incoming road
				// add density for all incoming traffic
				// and remove density for all outgoing traffic

				Road* thisRoad = intersections[i].roadsThatIntersect[j];
				int lightIndex = intersections[i].roadIndexToLight(j);
				int lightTime = currentLightTimes[i][lightIndex];

				// outgoing traffic
				// expected density removed
				intersections[i].roadDensity[j] -= minusDensities[i][j];

				// incoming traffic
				// the density leaving from the neighbor in that direction, only from the same direction
				// ASSUMPTION: left-right turns will even out
				bool neighborIntersectionFound = false;
				Intersection* neighborIntersection = &thisRoad->endPoints[0];
				for (int k = 0; k < thisRoad->endPoints.size(); k++) {
					if (&thisRoad->endPoints[k] != &intersections[i]) {
						neighborIntersection = &thisRoad->endPoints[k];
					}
				}

				if (neighborIntersectionFound) {
					int neighborIntersectionIndex = 0;
					for (int k = 0; k < intersections.size(); k++) {
						if (neighborIntersection == &intersections[k]) {
							neighborIntersectionFound = true;
							neighborIntersectionIndex = k;
						}
					}

					int neighborParallelRoadIndex;
					for (int k = 0; k < neighborIntersection->roadsThatIntersect.size(); k++) {
						if (neighborIntersection->roadsThatIntersect[k]->name == thisRoad->name
							&& neighborIntersection->roadsThatIntersect[k]->id != thisRoad->id) {

							neighborParallelRoadIndex = k;
						}
					}

					int otherLightIndex = intersections[neighborIntersectionIndex].roadIndexToLight(neighborParallelRoadIndex);
					int otherLightTime = currentLightTimes[neighborIntersectionIndex][otherLightIndex];
					// expected density added from neighbor parallel road -- from the Intersection connected to the same Road as this one
					// the Road opposite to this one at the same intersection was found. But since they share light time, non-opposite would work too
					intersections[i].roadDensity[j] += minusDensities[neighborIntersectionIndex][neighborParallelRoadIndex];
				}
				//else if (DEBUG_MODE) {
				//	cout << "No neighbor intersection found" << endl;
				//}
			}
		}
	}

	// finds possibly optimal light times for all lights at all intersections
	// uses a branch-and-bound method over multiple iterations
	// time complexity O(i * a lot). probably O(n^3) or worse.
	vector<vector<int>> lightOptimization() {
		struct Node {
			Map* parent;
			vector<Intersection> intersections;
			vector<vector<int>> lightTimes;
			float potentialFlow = 0;
			int currentIntersection = 0;

			float roadAverageMultiplier = 0.75;

			Node(Map* parent, vector<Intersection> intersections, vector<vector<int>> lightTimes, int currentIntersection) {
				this->parent = parent;
				this->intersections = intersections;
				this->lightTimes = lightTimes;
				this->currentIntersection = currentIntersection;
				this-> potentialFlow = findPotentialFlow();
			}

			// use traffic density to find sum of ALL cars passing through ALL intersections over 1 unit of time
			float findPotentialFlow() {
				float density = 0;
				for (int i = 0; i < currentIntersection; i++) {
					float averageDensity = 0;
					float sumDensity = 0;
					int numRoads = 0;

					for (int j = 0; j < lightTimes[i].size(); j++) {
						for (int k = 0; k < intersections[i].togetherRoads[j].density.size(); k++) {
							sumDensity += intersections[i].togetherRoads[j].density[k];
							numRoads++;
						}
					}
					averageDensity = sumDensity / numRoads;

					for (int j = 0; j < lightTimes[i].size(); j++) {
						float roadDensity = 0;
						for (int k = 0; k < intersections[i].togetherRoads[j].density.size(); k++) {
							roadDensity += intersections[i].togetherRoads[j].density[k] - averageDensity * roadAverageMultiplier;
						}
						density += getCarsPassing(lightTimes[i][j]) * intersections[i].laneList[j] * roadDensity;
					}
				}

				// for lights not yet determined, use ideal flow
				// assume rest are in the ideal state
				// since cars passing should accout for cars speeding up before they can move,
				// ideal would be if each light were max time and there was an exact amount of cars to move through the light for max light time
				/*for (int i = currentIntersection; i < intersections.size() ; i++) {
					for (int j = 0; j < lightTimes[i].size(); j++) {
						density += getCarsPassing(SETTINGS.maximumLightTime) * intersections[i].laneList[j];
					}
				}
				*/
				return density * SETTINGS.densityToCarRatio;
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

		// set starter values as average between min and max allowed
		vector<vector<int>> initialLightTimes;
		float averageLightTime = SETTINGS.defaultLightValue;
		for (int i = 0; i < intersections.size(); i++) {
			vector<int> singleLightTime(intersections[i].togetherRoads.size(), averageLightTime); /*number of light times as connected roads*/
			initialLightTimes.push_back(singleLightTime);
		}

		Node initial(this, intersections, initialLightTimes, 0);
		pq.push(initial);
		int totalNodes = 1;

		vector<vector<int>> resultLightTimes = initial.lightTimes;

		vector<vector<vector<int>>> allChangeSets;
		for (int i = 0; i < intersections.size(); i++) {
			int lightCount = initial.lightTimes[i].size();
			vector<int> changesSet(lightCount, 0);
			tempSolutions.clear();
			getLightChanges(changesSet, 0); //backtracking  // results explained at that function
			allChangeSets.push_back(tempSolutions);
		}

		if (intersections.size() > 0) {
			for (int iter = 0; iter < SETTINGS.iterations; iter++) {
				while (!pq.empty()) {
					Node currentNode = pq.top();
					pq.pop();

					// add more nodes to pq
					// ensures it hasn't passed max number of intersections
					if (currentNode.currentIntersection < currentNode.lightTimes.size()) {
						if (DEBUG_MODE) {
							cout << "Size of possible choices: " << allChangeSets[currentNode.currentIntersection].size() << endl;
							cout << "Total Nodes: " << totalNodes << endl;
							cout << "Current Intersection: " << currentNode.currentIntersection << endl;
						}

						// place each possible solution into a Node in the PQ
						for (int i = 0; i < allChangeSets[currentNode.currentIntersection].size(); i++) {
							vector<vector<int>> newLightTimes = currentNode.lightTimes;
							bool valid = true;
							bool allGreaterZero = true;
							for (int j = 0; j < allChangeSets[currentNode.currentIntersection][i].size(); j++) {
								newLightTimes[currentNode.currentIntersection][j] += allChangeSets[currentNode.currentIntersection][i][j];
								if (allChangeSets[currentNode.currentIntersection][i][j] <= 0) {
									allGreaterZero = false;
								}
								if (newLightTimes[currentNode.currentIntersection][j] < SETTINGS.minimumLightTime
									|| newLightTimes[currentNode.currentIntersection][j] > SETTINGS.maximumLightTime) {
									valid = false;
								}
							}
							if (allGreaterZero && iter > 0) {
								allGreaterZero = false;
							}
							if (valid) {
								Node newNode(this, intersections, newLightTimes, currentNode.currentIntersection + 1);
								pq.push(newNode);
								totalNodes++;
							}
						}
					}

					// if the current node is a potential solution (reached last intersection)
					// and the top node is no more promising than the current Node,
					// then a solution has been found
					if (pq.empty() || currentNode.currentIntersection == intersections.size() && pq.top().potentialFlow <= currentNode.potentialFlow) {
						// lightTimes belonging to the best node
						resultLightTimes = currentNode.lightTimes;
						break;
					}
				}

				if (DEBUG_MODE) {
					cout << "Optimization iteration [" << iter << "] ran using Node Count: " << totalNodes << endl;
				}

				// spread density
				doSpreadDensity(resultLightTimes);

				// effectively a clear
				pq = priority_queue<Node, vector<Node>, LessThanByPromising>();
				Node nextInitial(this, intersections, resultLightTimes, 0);
				pq.push(nextInitial);
				totalNodes = 1;
			}
		}

		return resultLightTimes;
	}

	// returns a pointer to a road with the given name and id, nullptr if none exists
	Road * getroadPntr(string name, string id) {	//Returns pointer to matching road stored within roads vector, Nullptr if no match. So that Input can construct Intersections from roads within map!
													//Requirements and Suggestions from Dwight
		for (int i = 0; i < roads.size(); i++) {
			if (roads[i].name == name && roads[i].id == id) {
				return &roads[i];
			}
		}

		return nullptr;
	}

	// compares name and id of Intersection to check if it already exists
	bool roadExists(Road * road) {
		for (int i = 0; i < roads.size(); i++) {
			if (roads[i].name == road->name && roads[i].id == road->id) {
				return true;
			}
		}

		return false;
	}

	// compares name of Intersection to check if it already exists
	bool intersectExists(Intersection * intersection) {
		for (int i = 0; i < intersections.size(); i++) {
			if (intersections[i].name == intersection->name) {
				return true;
			}
		}

		return false;
	}
};

// used when debugging to display
// resulting light times and final density on screen
void printLightTimings(Map map, vector<vector<int>> lightTimings) {
	for (int i = 0; i < lightTimings.size(); i++) {
		cout << "Intersection: " << map.intersections[i].name << endl;
		cout << "Times:" << endl;
		for (int j = 0; j < lightTimings[i].size(); j++) {
			cout << "\t" << map.intersections[i].togetherRoads[j].name << ": " << lightTimings[i][j] << endl;
		}

		cout << "Final Density:" << endl;
		for (int j = 0; j < map.intersections[i].roadDensity.size(); j++) {
			cout << "\t" << map.intersections[i].roadsThatIntersect[j]->name << " - " << map.intersections[i].roadsThatIntersect[j]->id << ": " << map.intersections[i].roadDensity[j] << endl;
		}
	}
};

// class written by Dwight Herman
class Input {	//NEEDS TO BE CONTAINED WITHIN A TRY BLOCK. WILL THROW STRINGS IF PARSING OR ANYTHING ELSE FAILS!
	
	typedef void(Settings::*paramSet)(float);	//function pointer to Settings method to set a specific parameter.

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
	
	float trafficdensity() {	//parse trafficdensity non-terminal of input grammar; returns integer of traffic density

		char feed[20];	//characters in FLT_MAX + 4 for nullsequence and cleanliness

		input.get(feed, 16, ',');

		return stof(feed, nullptr);
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

			blueprint = new Road(name, id, time);
			
			if (map->roadExists(blueprint))
				throw string(name + " " + id + " is duplicate!");
			
			map->importRoads(*blueprint);	//DOES NOT CURRENTLY CHECK IF ROAD ALREADY EXISTS

			delete blueprint;
		}
		catch (string s){	//If duplicate road is found.

			throw;
		}
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
		vector<int>lanelist;
		float density;
		vector<float> densitylist;
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

					if (road == nullptr)
						throw string(rname + " " + rid + " is not defined!");

					roadlist.push_back(road);
					lanelist.push_back(lanes);
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

			blueprint = new Intersection(iname, roadlist, lanelist, densitylist);

			if (map->intersectExists(blueprint))
				throw string(iname + " is duplicate!");
			
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
			throw string("Input file does not contain ':ROADS'!");

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
			throw string("Input file does not contain ':INTERSECTIONS'!");
		
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
				throw string("Input file does not contain ':BEGIN'!");

			data();

			input.getline(feed, 10);

			if (string(feed) != ":END,,,")
				throw string("Input file does not contain ':END'!");
	}

	paramSet pid(string & input) {		//returns function pointer of proper Map method given input

		const string pid[PARAM_NUM] = { MIN_PID, MAX_PID, DEFAULT_PID, STARTUP_PID, PASSING_PID, 
										DENSITY_PID, TRAVEL_PID, CHANGE_PID, ITERATIONS_PID };
		const paramSet func[PARAM_NUM] = {	&Settings::setMin, &Settings::setMax, &Settings::setDefaultLightValue, 
											&Settings::setCarStartupTime, &Settings::setCarPassingRate, 
											&Settings::setDensityToCarRatio, &Settings::setTravelTimeDensityMultiplier, 
											&Settings::setLightTimeTestValueChange, &Settings::setIterations };
		
		for (int i = 0; i < PARAM_NUM; i++)
		{
			if (input.substr(0, pid[i].size()) == pid[i]) {

				input = input.substr(pid[i].size(), input.size());

				return func[i];
			}
		}

		throw string("Invalid Parameter Specifier!");
	}

	float value(string & input) {		//returns float if input can be converted into one
	
		try {

			return stof(input, nullptr);
		}
		catch (...) {

			throw string("Invalid Number!");
		}
	}

	void parseParam(string & input) {		//to parse input from keyboard during runtime

		paramSet p = pid(input);

		if (input[0] != ' ')
			throw string("No space after Parameter Specifier!");
		strPopFront(input);

		(&SETTINGS->*p)(value(input));
	}

public:
	Input(string inputPath, Map &m) {								//constructor that takes filepath of .csv file and Map object to store input into
		
		path = inputPath;
		
		input.exceptions(ifstream::failbit | ifstream::badbit);		//sets input to throw exceptions for logical or read errors
		input.open(path);

		map = &m;
	}
	~Input()
	{		
		if (input.is_open())
			input.close();
	}
	bool paramQuery(string input) {		//to begin parsing of parameter query 'input', will return true if more queries can be made

		if (input == CALC_INIT) {
			parseInput();
			return false;
		}
		else {
			try {
				parseParam(input);	
			}
			catch (string s) {
				cout << s << endl;
			}
			catch (...) {
				cout << "Invalid Parameter!" << endl;
				throw;
			}
			return true;
		}
	}
};

// class written by Dwight Herman
class Output {	//NEEDS TO BE CONTAINED WITHIN A TRY BLOCK. WILL THROW IF OUTPUT FAILS!

private:
	Map * map;						//pointer to Map object to pull data from
	string path;					//path to output .csv file
	ofstream output;				//output stream to .csv file
public:
	Output(string outpath, Map &m) {		//constructor that takes filepath of .csv file to store data into and map object to pulldata from

		path = outpath;

		output.exceptions(ifstream::failbit | ifstream::badbit);		//sets input to throw exceptions for logical or read errors
		output.open(path, ofstream::app);								//appends output

		map = &m;
	}
	void outputToFilePath(vector<vector<int>> cycleTimings) {			//prints results to filestream output

		output << ":BEGIN," << endl << ":INTERSECTIONS," << endl;

		for (int i = 0; i < cycleTimings.size(); i++) {

			output << ':' << map->intersections[i].name << ',' << endl;

			for (int j = 0; j < cycleTimings[i].size(); j++) {

				output << map->intersections[i].togetherRoads[j].name << ',' << cycleTimings[i][j] << endl;

			}
		}

		output << ":END" << endl;
	}
};

void main() {
	//HOW TO USE INPUT:
	Map map;			//to store Input data
	string keyin;		//to store keyboard input
	bool fail;			//to exit input loop
	Input * input;		//pointer to input; so Input object can be recreated easily.
	Output * output;	//pointer to output; so Output object can be recreated easily.

	cout << "/---------------------------------/" << endl;
	cout << "/-TRAFFIC CONTROL IMPLEMENTATIONS-/" << endl;
	cout << "/--TRAFFIC LIGHT CYCLE OPTIMIZER--/" << endl;
	cout << "/---------------------------------/" << endl;
	cout << endl;
	
	do													
	{	
		fail = false;									//Assume all will be well
		input = nullptr;								//In case Input object fails at construction

		cout << "Enter input filepath: ";				//prompt user for Filepath

		getline(cin, keyin);							//take keyboard input; getline is best because it will incorporate spaces!

		try {											//Input will throw if parsing fails

			input = new Input(keyin, map);				//create new Input object with user entered filepath

			do {										//Allow user as many queries as needed. (Parameter changes or calculation initialization)

				getline(cin, keyin);					//take keyboard input; getline is best because it will incorporate spaces!

			} while (input->paramQuery(keyin));			//have Input object query keyin; if more queries can be made, it will return true and repeat the loop.
		}
		catch (string s) {								//Strings will be thrown if failed for known reason

			cout << s << endl;							//Tell user why parsing failed

			fail = true;								//Set input loop to repeat
		}
		catch (...) {									//To catch all other exceptions, mostly from std library functions

			cout << "Error while reading file!" << endl;//Unknown error

			fail = true;								//Set input loop to repeat
		}

		delete input;									//Destroy input when finished using it

	} while (fail);										//Allow user to retry if file fails
		
	//INPUT IS FINISHED AND SUCCESSFUL PAST THIS LINE. MAP CALCULATIONS CAN NOW BE INITIALIZED AND OUTPUT MADE

	cout << "Calculating Optimal Light Timing...";

	//clock_t startTime = clock();
	vector<vector<int>> lightTimings = map.lightOptimization();
	cout << "Done!" << endl;
	//clock_t runTime = clock() - startTime;
	//cout << "Run time: " << runTime << " clock ticks" << endl;

	if (DEBUG_MODE) {
		printLightTimings(map, lightTimings);
	}

	do
	{
		fail = false;									//Assume all will be well
		output = nullptr;								//In case Output object fails at construction

		cout << "Enter output filepath: ";				//prompt user for Filepath

		getline(cin, keyin);							//take keyboard input; getline is best because it will incorporate spaces!

		try {

			output = new Output(keyin, map);			//create new Output object with user entered filepath

			output->outputToFilePath(lightTimings);		//attempt to output results to file

		}
		catch (...) {									//To catch all exceptions, mostly from std library functions

			cout << "Error while writing file!" << endl;//Uknown Error

			fail = true;								//set output loop to repeat
		}

		delete output;

	} while (fail);

	//OUTPUT IS FINISHED AND SUCCESSFUL PAST THIS LINE.

	cout << "Closing..." << endl;
}

inline void strPopFront(string & input) {	//removes first character of 'input'
	
	input = input.substr(1, input.size());
}