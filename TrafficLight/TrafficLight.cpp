#include <iostream>
#include <vector>
#include <queue>

using namespace std;

class Intersection;

class Road {
private:
	string name;
	int travelTime;
	vector<Intersection> endPoints;
	void setEndPoint(Intersection point); // located following Intersection class definition
public:
	Road(string name, int travelTime) {
		name = name;
		travelTime = travelTime;
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
	Intersection(string name, vector<Road*>roads, vector<int>density) {
		name = name;
		roadsThatIntersect = roads;
		roadDensity = density;
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

	int findPotentialFlow(vector<vector<int>> lightTimes) {
		// use traffic density to find sum of all cars passing through all intersections over time 't'
		// for lights not yet determined, use ideal flow
		return 0;
	}
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
			vector<vector<int>> lightTimes;
			int potentialFlow = 0;
			int currentIntersection = 0;
		};
		struct LessThanByPromising {
			bool operator()(const Node& lhs, const Node& rhs) const
			{
				return lhs.potentialFlow < rhs.potentialFlow;
			}
		};

		// PQ of node potential timings
		priority_queue<Node, vector<Node>, LessThanByPromising> pq;
		Node initial;
		for (int i = 0; i < intersections.size(); i++) {
			vector<int> singleLightTime(intersections[i].roadsThatIntersect.size(), 0); /*TODO: div 2*/
			initial.lightTimes.push_back(singleLightTime);
		}
		initial.potentialFlow = findPotentialFlow(initial.lightTimes);
		pq.push(initial);

		if (intersections.size() > 0) {
			while (!pq.empty()) {
				Node currentNode = pq.top();
				pq.pop();

				// add more nodes to pq
				// for each light at the current intersection, try adding 5 to it
				// TODO: +- 5 or the same
				// TODO: multiple light changes in the same Node
				if (currentNode.currentIntersection < currentNode.lightTimes.size()) {
					int lightCount = currentNode.lightTimes[currentNode.currentIntersection].size();
					for (int i = 0; i < lightCount; i++) {
						Node newNode;
						newNode.currentIntersection = currentNode.currentIntersection + 1;
						newNode.lightTimes = currentNode.lightTimes;
						newNode.lightTimes[currentNode.currentIntersection][i] += 5;
						newNode.potentialFlow = findPotentialFlow(newNode.lightTimes);
						pq.push(newNode);
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