#include "MyLib/MyTimer.h"
#include "MyLib/ParaReader.h"
#include "graph/Graph.h"
#include <iostream>
#include <set>
#include <stdio.h>
#include <random>

struct ConfigParams {
    char graphFilePath[200];
    char updateFilePath[200];
    double rhoValue;
};

struct ConfigParams parseCommandLineArgs(int argc, char **argv) {
    ConfigParams result;
    int argIndex = 1;
    bool parseError = false;
    char *currentArg;
    int charPos;
    char paramName[10];
    char graphPath[200] = "./test_data/Condmat.bin";
    char updatePath[200] = "./test_data/10x_Uniform_5.txt";
    double rhoParam = 0.01;

    printf("The input parameters are:\n\n");
    while (argIndex < argc && !parseError) {
        currentArg = argv[argIndex++];
        if (argIndex == argc) {
            parseError = true;
            break;
        }
        charPos = getNextChar(currentArg);
        if (currentArg[charPos] != '-') {
            parseError = true;
            break;
        }
        getNextWord(currentArg + charPos + 1, paramName);
        printf("%s\t", paramName);
        currentArg = argv[argIndex++];
        if (strcmp(paramName, "graph") == 0) {
            getNextWord(currentArg, graphPath);
            printf("%s\n", graphPath);
        } else if (strcmp(paramName, "rho") == 0) {
            rhoParam = atof(currentArg);
            if (rhoParam < 0 || rhoParam > 1) {
                parseError = true;
                break;
            }
            printf("rho : %lf\n", rhoParam);
        } else if (strcmp(paramName, "update") == 0) {
            getNextWord(currentArg, updatePath);
            printf("%s\n", updatePath);
        } else {
            parseError = true;
            printf("Unknown option -%s!\n\n", paramName);
        }
    }

    /*****************************************************************************/
    strcpy(result.graphFilePath, graphPath);
    strcpy(result.updateFilePath, updatePath);
    result.rhoValue = rhoParam;
    return result;
}

void usage() {
    printf("Usage:\n");
    printf("dynstrclu -graph [graph file] -update [update file] "
           "-rho [\\rho]\n");
}

int generateRandomInteger(int lowerBound, int upperBound) {
    std::random_device randomDevice;
    std::mt19937 generator(randomDevice());
    std::uniform_int_distribution<int> distribution(lowerBound, upperBound);

    return distribution(generator);
}

void loadGraphFromFile(const char *filePath, unsigned int &vertexCount, 
                       unsigned int &edgeCount, int *&edgeArray) {
    FILE *fileHandle = fopen(filePath, "rb");
    if (fileHandle == NULL) {
        printf("graph file not found.\n");
        exit(1);
    }
    fread(&vertexCount, 1, sizeof(int), fileHandle);
    fread(&edgeCount, 1, sizeof(int), fileHandle);
    edgeArray = (int *) malloc(sizeof(int) * edgeCount);
    fread(edgeArray, edgeCount, sizeof(int), fileHandle);
    fclose(fileHandle);
}

void loadUpdatesFromFile(const char *filePath, vector<pair<int, pair<int, int>>> &updateList) {
    FILE *fileHandle = fopen(filePath, "r");
    int opType = 0, vertex1 = 0, vertex2 = 0;
    while (fscanf(fileHandle, "%d%d%d", &opType, &vertex1, &vertex2) != EOF) {
        updateList.emplace_back(make_pair(opType, make_pair(vertex1, vertex2)));
    }
    fclose(fileHandle);
}

void initializeVertexList(int vertexCount, MyVector<dynscan::Vertex *> &vertexList) {
    vertexList.reserve(vertexCount);
    for (int idx = 0; idx < vertexCount; idx++) {
        dynscan::Vertex *newVertex = new dynscan::Vertex(idx + 1);
        vertexList.push_back(newVertex);
    }
}

void processInitialEdges(Graph &graph, int *edgeArray, unsigned int edgeCount) {
    double startTime = getCurrentTime();
    double endTime = 0;
    double outputInterval = 0.1;
    double nextOutputPercent = outputInterval;
    int nextOutputPos = (edgeCount / 2) * nextOutputPercent;
    
    for (int edgeIdx = 0, processedCount = 0; edgeIdx < edgeCount; edgeIdx += 2, ++processedCount) {
        graph.insertEdge(edgeArray[edgeIdx], edgeArray[edgeIdx + 1]);
        if (processedCount == nextOutputPos - 1) {
            endTime = getCurrentTime();
            printf("Total time used after inserting %.2lf m edges: *%.9lf*\t"
                   "Average time in processing one insertion: *%.9lf*\n",
                   nextOutputPercent, endTime - startTime,
                   (endTime - startTime) / (double) (processedCount + 1));
            nextOutputPercent += outputInterval;
            nextOutputPos = (edgeCount / 2) * nextOutputPercent;
        }
    }
}

void processUpdatesAndQueries(Graph &graph, vector<pair<int, pair<int, int>>> &updateList,
                              unsigned int edgeCount, unsigned int vertexCount) {
    printf("---------------------------------------------------------------------\n");
    int nextOutputPos = (int) (0.1 * edgeCount / 2);
    double totalQueryTime = 0, totalUpdateTime = 0;
    int queryCount = 0;
    int nextQueryPos = 20;
    
    for (long long updateIdx = 0, updateSize = updateList.size(); updateIdx < updateSize; ++updateIdx) {
        double startTime = getCurrentTime();
        if (updateList[updateIdx].first == 1) {
            graph.insertEdge(updateList[updateIdx].second.first, 
                            updateList[updateIdx].second.second);
        } else {
            graph.removeEdge(updateList[updateIdx].second.first, 
                            updateList[updateIdx].second.second);
        }
        double endTime = getCurrentTime();
        totalUpdateTime += endTime - startTime;
        
        if (updateIdx == nextOutputPos - 1) {
            double outputPercent = nextOutputPos / (double) (edgeCount / 2);
            printf("After updating %.2lf m edges, the average time for each update: *%.9lf*\n",
                   outputPercent, totalUpdateTime / (double) (updateIdx + 1));
            nextOutputPos += (int) (0.1 * edgeCount / 2);
        }
        
        if (updateIdx == nextQueryPos - 1) {
            double queryTime = 0;
            double epsilon = generateRandomInteger(100, 1000) / 1000.0;
            int muParam = generateRandomInteger(1, int(2 * edgeCount / vertexCount));
            queryTime = graph.query(epsilon, muParam);
            totalQueryTime += queryTime;
            queryCount++;
            nextQueryPos += nextQueryPos;
        }
    }

    printf("---------------------------------------------------------------------\n");
    printf("Average query time: *%.9lf*\t", totalQueryTime / queryCount);
}

int main(int argc, char **argv) {
    if (argc <= 2) {
        usage();
        return 0;
    }
    printf("Start to parse the arguments\n");

    ConfigParams config = parseCommandLineArgs(argc, argv);
    printf("Arguments parsed.\n");

    unsigned int vertexCount, edgeCount;
    int *edgeArray = nullptr;
    loadGraphFromFile(config.graphFilePath, vertexCount, edgeCount, edgeArray);

    vector<pair<int, pair<int, int>>> updateList;
    updateList.reserve(9 * (edgeCount / 2));
    loadUpdatesFromFile(config.updateFilePath, updateList);

    MyVector<dynscan::Vertex *> vertexList;
    initializeVertexList(vertexCount, vertexList);
    
    Graph graph(vertexList, config.rhoValue);
    printf("Graph generation finished with %d vertices and %d edges.\n", 
           vertexCount, edgeCount / 2);

    vertexList.release_space();
    processInitialEdges(graph, edgeArray, edgeCount);
    processUpdatesAndQueries(graph, updateList, edgeCount, vertexCount);

    // 输出最终的聚类信息
    //double final_eps = 0.5;  // 你可以调整这些参数
    //int final_mu = 2;        // 你可以调整这些参数
    //graph.printFinalClusterInfo(final_eps, final_mu);
    
    free(edgeArray);
    return 0;
}
