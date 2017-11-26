#include <iostream>
#include <vector>
#include <queue>
#include <string>

using namespace std;

class Intersection;

class Road {
private:
	//string name;
	int travelTime;
	vector<Intersection> endPoints;
	void setEndPoint(Intersection point); // located following Intersection class definition
public:
	string name;
	Road(string name, int travelTime) {
		this->name = name;
		travelTime = travelTime;
	}
};

// class written by Robert Piepsney
class Intersection {
private:
	// assume roads with the same name lead the same way
	//vector<Road*> roadsThatIntersect;
	//string name;
	vector<int> roadDensity;
public:
	vector<Road*> roadsThatIntersect;
	string name;
	struct TogetherRoad {
		vector<Road*> roads;
		string name;
		//int time;
		TogetherRoad(Road* initialRoad) {
			roads.push_back(initialRoad);
			this->name = initialRoad->name;
		}
	};
	vector<TogetherRoad> togetherRoads;
	Intersection(string name, vector<Road*>roads, vector<int>density) {
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
};

void Road::setEndPoint(Intersection point) {
	endPoints.push_back(point);
}

// class written by Robert Piepsney
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
	void setMin(int min) {
		minimumLightTime = min;
	}
	void setMax(int max) {
		maximumLightTime = max;
	}

	vector<vector<int>> lightOptimization() {
		struct Node {
			vector<Intersection> intersections;
			vector<vector<int>> lightTimes;
			int potentialFlow = 0;
			int currentIntersection = 0;

			Node(vector<Intersection> intersections, vector<vector<int>> lightTimes, int currentIntersection) {
				this->intersections = intersections;
				this->lightTimes = lightTimes;
				this->currentIntersection = currentIntersection;
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
		float averageLightTime = (minimumLightTime + maximumLightTime) / 2;
		for (int i = 0; i < intersections.size(); i++) {
			// TODO: bound by min and max (set as average?)
			// change times to float?
			vector<int> singleLightTime(intersections[i].togetherRoads.size(), averageLightTime); /*number of light times as connected roads*/
			initialLightTimes.push_back(singleLightTime);
		}

		Node initial(intersections, initialLightTimes, 0);
		pq.push(initial);
		int totalNodes = 1;

		if (intersections.size() > 0) {
			while (!pq.empty()) {
				Node currentNode = pq.top();
				pq.pop();

				// add more nodes to pq
				// for each light at the current intersection, try adding 5 to it
				// TODO: multiple light changes in the same Node
				// right now each node only changes 1 light

				// ensures it hasn't passed max number of intersections
				if (currentNode.currentIntersection < currentNode.lightTimes.size()) {
					int lightCount = currentNode.lightTimes[currentNode.currentIntersection].size();
					for (int i = 0; i < lightCount; i++) {
						for (int j = -5; j <= 5; j = j + 5) { // do for -5, 0, 5
							vector<vector<int>> newLightTimes = currentNode.lightTimes;
							newLightTimes[currentNode.currentIntersection][i] += j;
							Node newNode(intersections, newLightTimes, currentNode.currentIntersection + 1);
							pq.push(newNode);
							totalNodes++;
						}
					}
				}

				// if the current node is a potential solution (reached last intersection)
				// and the top node is no more promising than the current Node,
				// then a solution has been found
				if (currentNode.currentIntersection == intersections.size() && pq.top().potentialFlow <= currentNode.potentialFlow) {
					// lightTimes belonging to the best node
					cout << "Optimization iteration ran using Node Count: " << totalNodes << endl;
					return currentNode.lightTimes;
				}
			}
		}

		cout << "Optimization iteration ran using Node Count: " << totalNodes << endl;

		return initial.lightTimes; 
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
	for (int i = 0; i < lightTimings.size(); i++) {
		cout << "Intersection: " << map.intersections[i].name << endl;
		for (int j = 0; j < lightTimings[i].size(); j++) {
			cout << "\t" << map.intersections[i].togetherRoads[j].name << ": " << lightTimings[i][j] << endl;
		}
	}
	cout << "finished computation";
}