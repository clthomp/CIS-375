#include <iostream>
#include <vector>
#include <queue>
#include <string>
#include <algorithm> 

using namespace std;

bool DEBUG_MODE = true;

struct Settings {
	int minimumLightTime = 15;
	int maximumLightTime = 60;

	// time it takes before cars begin moving after a light turns green
	float carStartupTime = 3;
	// average number of cars that can pass through a light per 1 second after cars have sped up
	float carPassingRate = 1.0;
	// how many cars per 1 unit of density
	float densityToCarRatio = 1.0;
	// the importance travel time has on shifting density
	float travelTimeDensityMultiplier = 1.0;

	// how much to try moving each light time up or down at each iteration
	int lightTimeTestValueChange = 1;
};
Settings SETTINGS;

class Intersection;

// class written by Robert Piepsney
class Road {
private:

public:
	string name;
	string id;
	float travelTime;
	int mainLanes;
	vector<Intersection> endPoints;
	Road(string name, string id, float travelTime, int mainLanes) {
		this->name = name;
		this->id = id;
		this->travelTime = travelTime;
		this->mainLanes = mainLanes;
	}
	void setEndPoint(Intersection point); // located following Intersection class definition
};

// class written by Robert Piepsney
class Intersection {
private:

public:
	vector<Road*> roadsThatIntersect;
	string name;
	vector<float> roadDensity; // parallel to roadsThatIntersect
	struct TogetherRoad {
		vector<Road*> roads;
		string name;
		TogetherRoad(Road* initialRoad) {
			roads.push_back(initialRoad);
			this->name = initialRoad->name;
		}
	};
	vector<TogetherRoad> togetherRoads;
	Intersection(string name, vector<Road*>roads, vector<float>density) {
		this->name = name;
		roadsThatIntersect = roads;
		roadDensity = density;

		findTogetherRoads();
	}

	void findTogetherRoads() {
		// unite all roads of the same name
		for (int i = 0; i < roadsThatIntersect.size(); i++) {
			bool isFound = false;
			for (int j = 0; j < togetherRoads.size(); j++) {
				if (roadsThatIntersect[i]->name == togetherRoads[j].name) {
					isFound = true;
					(togetherRoads[j].roads).push_back(roadsThatIntersect[i]);
				}
			}

			if (!isFound) {
				togetherRoads.push_back(TogetherRoad(roadsThatIntersect[i]));
			}
		}
	}

	int roadIndexToLight(int roadIndex) {
		// assume roadIndex is an index in roadsThatIntersect, return its index in togetherRoads
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

void Road::setEndPoint(Intersection point) {
	endPoints.push_back(point);
}

float getCarsPassing(float t) {
	t -= SETTINGS.carStartupTime;
	t *= SETTINGS.carPassingRate;
	return max(float(0.0), t);
}

// class written by Robert Piepsney
class Map {
private:
	vector<Road> roads;
public:
	vector<Intersection> intersections;
	void importRoads(Road inputRoads) {
		roads.push_back(inputRoads);
	}
	void importIntersections(Intersection inputIntersections) {
		intersections.push_back(inputIntersections);
	}

	vector<vector<int>> tempSolutions;

	// recursive method to generate all possible changes for light times
	// a light time can either increase, decrease, or stay the same by the value given from SETTINGS
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

	void doSpreadDensity(vector<vector<int>> currentLightTimes) {
		// for all the intersections, change density based on cars coming from other intersections of incoming roads
		for (int i = 0; i < intersections.size(); i++) {
			for (int j = 0; j < intersections[i].roadDensity.size(); j++) {
				// for the density of each incoming road
				// add density for all incoming traffic
				// remove density for all outgoing traffic
				Road* thisRoad = intersections[i].roadsThatIntersect[j];

				// outgoing traffic
				// find time for a road's light
				int lightIndex = intersections[i].roadIndexToLight(j);
				int lightTime = currentLightTimes[i][lightIndex];
				intersections[i].roadDensity[j] -= getCarsPassing(lightTime) * SETTINGS.densityToCarRatio *
													float(thisRoad->mainLanes) /
													(thisRoad->travelTime * SETTINGS.travelTimeDensityMultiplier);

				// incoming traffic
				// the density leaving from the neighbor in that direction, only from the same direction
				// assumption that turns will even out
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
					intersections[i].roadDensity[j] += getCarsPassing(otherLightTime) * SETTINGS.densityToCarRatio * 
														float(neighborIntersection->roadsThatIntersect[neighborParallelRoadIndex]->mainLanes) /
														(thisRoad->travelTime * SETTINGS.travelTimeDensityMultiplier);
				}
				else {
					cout << "No neighbor intersection found";
				}
			}
		}
	}

	vector<vector<int>> lightOptimization() {
		struct Node {
			Map* parent;
			vector<Intersection> intersections;
			vector<vector<int>> lightTimes;
			float potentialFlow = 0;
			int currentIntersection = 0;

			Node(Map* parent, vector<Intersection> intersections, vector<vector<int>> lightTimes, int currentIntersection) {
				this->parent = parent;
				this->intersections = intersections;
				this->lightTimes = lightTimes;
				this->currentIntersection = currentIntersection;
				this-> potentialFlow = findPotentialFlow();
			}

			float findPotentialFlow() {
				// use traffic density to find sum of all cars passing through all intersections over time 't'
				float density = 0;
				for (int i = 0; i < currentIntersection; i++) {
					for (int j = 0; j < lightTimes[i].size(); j++) {
						density += getCarsPassing(lightTimes[i][j]) * intersections[i].roadsThatIntersect[j]->mainLanes;
					}
				}

				// for lights not yet determined, use ideal flow
				// assume rest are in the ideal state
				// since cars passing should accout for cars speeding up before they can move,
				// ideal would be if each light were max time and there was an exact amount of cars to move through the light for max light time
				for (int i = currentIntersection; i < intersections.size() ; i++) {
					for (int j = 0; j < lightTimes[i].size(); j++) {
						density += getCarsPassing(SETTINGS.maximumLightTime) * intersections[i].roadsThatIntersect[j]->mainLanes;
					}
				}
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

		vector<vector<int>> initialLightTimes;
		float averageLightTime = (SETTINGS.minimumLightTime + SETTINGS.maximumLightTime) / 2;
		for (int i = 0; i < intersections.size(); i++) {
			// set as average
			vector<int> singleLightTime(intersections[i].togetherRoads.size(), averageLightTime); /*number of light times as connected roads*/
			initialLightTimes.push_back(singleLightTime);
		}

		Node initial(this, intersections, initialLightTimes, 0);
		pq.push(initial);
		int totalNodes = 1;

		vector<vector<int>> resultLightTimes = initial.lightTimes;

		if (intersections.size() > 0) {
			int iterations = 1;
			for (int iter = 0; iter < iterations; iter++) {
				while (!pq.empty()) {
					Node currentNode = pq.top();
					pq.pop();

					// add more nodes to pq
					// ensures it hasn't passed max number of intersections
					if (currentNode.currentIntersection < currentNode.lightTimes.size()) {
						int lightCount = currentNode.lightTimes[currentNode.currentIntersection].size();
						vector<int> changesSet(lightCount, 0);
						tempSolutions.clear();
						getLightChanges(changesSet, 0); //backtracking  // results explained at that function
						cout << "Size of thing: " << tempSolutions.size() << endl;
						for (int i = 0; i < tempSolutions.size(); i++) {
							vector<vector<int>> newLightTimes = currentNode.lightTimes;
							for (int j = 0; j < tempSolutions[i].size(); j++) {
								newLightTimes[currentNode.currentIntersection][j] += tempSolutions[i][j];
							}
							Node newNode(this, intersections, newLightTimes, currentNode.currentIntersection + 1);
							pq.push(newNode);
							totalNodes++;
						}
					}

					// if the current node is a potential solution (reached last intersection)
					// and the top node is no more promising than the current Node,
					// then a solution has been found
					if (currentNode.currentIntersection == intersections.size() && pq.top().potentialFlow <= currentNode.potentialFlow) {
						// lightTimes belonging to the best node
						resultLightTimes = currentNode.lightTimes;
						break;
					}
				}

				if (DEBUG_MODE) {
					cout << "Optimization iteration ran using Node Count: " << totalNodes << endl;
				}

				// spread density
				doSpreadDensity(resultLightTimes);

				// effectively a clear
				pq = priority_queue<Node, vector<Node>, LessThanByPromising>();
				Node nextInitial(this, intersections, resultLightTimes, 0);
				pq.push(initial);
				totalNodes = 1;
			}
		}

		return resultLightTimes;
	}
};

void main() {
	Map map;
	SETTINGS.minimumLightTime = 15;
	SETTINGS.maximumLightTime = 30;

	Road roadA1 = Road("A", "1", 5, 1);
	Road roadA2 = Road("A", "2", 5, 1);
	Road roadB1 = Road("B", "1", 5, 1);
	Road roadB2 = Road("B", "2", 5, 1);

	vector<Road*> roadList  = { &roadA1, &roadA2, &roadB1, &roadB2 };
	vector<float> densityList = { 1, 1, 1, 1 };

	Intersection intersectionAB = Intersection("A x B", roadList, densityList);

	roadA1.setEndPoint(intersectionAB);
	roadA2.setEndPoint(intersectionAB);
	roadB1.setEndPoint(intersectionAB);
	roadB2.setEndPoint(intersectionAB);

	map.intersections.push_back(intersectionAB);

	vector<vector<int>> lightTimings = map.lightOptimization();
	for (int i = 0; i < lightTimings.size(); i++) {
		cout << "Intersection: " << map.intersections[i].name << endl;
		for (int j = 0; j < lightTimings[i].size(); j++) {
			cout << "\t" << map.intersections[i].togetherRoads[j].name << ": " << lightTimings[i][j] << endl;
		}
	}
	cout << "finished computation";
}