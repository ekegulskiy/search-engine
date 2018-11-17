/** 
 *  @file    hw2.cpp
 *  @author  Eduard Kegulskiy
 *  @date    10/01/2018  
 *  @version 1.0
 *  
 *  @brief CSC 849, HW2
 *
 *  @section DESCRIPTION
 *  
 *  A mini-search engine that is able to do boolean and ranked search.
 *  For HW2, a ranked search is performed by default
 */

#include "SearchEngine.h"

const string PREDIFINED_QUERIES[] = {
    "nexus like love happy",
    "asus repair",
    "0(touch screen) fix repair",
    "1(great tablet) 2(tablet fast)",
    "tablet"
};

enum SEARCH_TYPE{
    SEARCH_BOOLEAN,
    SEARCH_RANKED
};

#define EXIT_KEY                'q'
#define CUSTOM_QUERY_KEY        '6'
#define TOGGLE_SEARCH_TYPE_KEY  't'

char displayIntro(){
    cout << "******************************************************" << endl;
    cout << "*        Welcome to the CSC 849 Search Engine!       *" << endl;
    cout << "* The following interactive program lets you execute *" << endl;
    cout << "* pre-defined queries, or specify any query you want *" << endl;
    cout << "******************************************************" << endl;
}

char displayMenu(SEARCH_TYPE searchType){
    char selection;

    cout  << "Select query below to execute " << (searchType == SEARCH_BOOLEAN ? "boolean" : "ranked") << " search..." << endl;

    for(unsigned int i =0; i < sizeof(PREDIFINED_QUERIES)/sizeof(string); i++){
        cout  << "[" << i+1 << "] - \"" << PREDIFINED_QUERIES[i] << "\"" << endl;
    }
    
    cout << "[6] - custom query..." << endl;
    cout << "[t] - toggle search type (boolean or ranked)" << endl;
    cout << "[q] - exit" << endl;

    cin >> selection;
    return selection;
}

void printRankedResults(string query, SCORES_LIST& scoresSet){
    cout << "QUERY: " << "\"" << query << "\"" << endl;
    cout << "RESULT: " << endl;

    if( scoresSet.size() > 0){
        for(SCORES_LIST::reverse_iterator iter = scoresSet.rbegin(); iter != scoresSet.rend(); ++iter)
        {
            cout << "DocID: " << iter->second << ", score=" << iter->first << endl;
        }

        cout << endl << endl;
    }
    else{
        cout << "no match found." << endl << endl;
    }
}

void printBooleanResults(string query, vector<unsigned long>& docIDs){
    cout << "QUERY: " << "\"" << query << "\"" << endl;
    cout << "RESULT: ";

    if( docIDs.size() > 0){
        cout << "match found in doc(s) ";
        for( unsigned int i = 0; i < docIDs.size(); i++){
            cout << docIDs[i] << ", ";
        }

        cout << endl << endl;
    }
    else{
        cout << "no match found." << endl << endl;
    }

}

int main(int argc, char *argv[])
{
    SearchEngine searchEngine;
    bool bIndexOnly = false;
    bool isSquad = false;
    string collectionPath = "collections/documents.txt";
    string squadTrainDataPath, squadDevDataPath;

    int argIndex = 1;
    while(argIndex < argc){
        string nextArg = argv[argIndex++];
        if(nextArg == "-index"){
            bIndexOnly = true;
        }
        else if(nextArg == "-squad-train-data"){
            isSquad = true;
            squadTrainDataPath = argv[argIndex++];
        }
        else if(nextArg == "-squad-dev-data"){
            isSquad = true;
            squadDevDataPath = argv[argIndex++];
        }
        else
        {  
            cout << "Invalid option" << endl;
            exit(-1);
        }
    }

    if(isSquad)
    {
        searchEngine.buildFromSquadData(squadTrainDataPath, true);
        searchEngine.buildFromSquadData(squadDevDataPath, true);
    }
    else
        searchEngine.buildFromFile(collectionPath);

    if(bIndexOnly){
        searchEngine.printIndex(false);
        return 0;
    }

    displayIntro();
    char selection;
    SEARCH_TYPE searchType = SEARCH_RANKED;
    
    do {
        selection = displayMenu(searchType);

        switch(selection)
        {
            case CUSTOM_QUERY_KEY:
            {
                string query; 
                cout << "Type the query and press ENTER" << endl;
                std::getline(std::cin, query); // read ENTER
                std::getline(std::cin, query); // read actual query
                if(searchType == SEARCH_BOOLEAN){
                    vector<unsigned long> searchResults = searchEngine.booleanSearch(query);
                    printBooleanResults(query, searchResults);
                }
                else{
                    SCORES_LIST scoresSet = searchEngine.rankedSearch(query);
                    printRankedResults(query, scoresSet);
                }
                break;
            }
            case '1': 
            case '2': 
            case '3': 
            case '4': 
            case '5': 
            {
                if(searchType == SEARCH_BOOLEAN){
                    vector<unsigned long> searchResults = searchEngine.booleanSearch(PREDIFINED_QUERIES[selection - '1']);
                    printBooleanResults(PREDIFINED_QUERIES[selection - '1'], searchResults);
                }
                else{
                    SCORES_LIST scoresSet = searchEngine.rankedSearch(PREDIFINED_QUERIES[selection - '1']);
                    printRankedResults(PREDIFINED_QUERIES[selection - '1'], scoresSet);
                }

                break;
            }
            case TOGGLE_SEARCH_TYPE_KEY: 
            {
                searchType = (searchType == SEARCH_BOOLEAN) ? SEARCH_RANKED : SEARCH_BOOLEAN;
                break;
            }
            case EXIT_KEY:
                cout << "Good bye!" << endl;
                break;
            default: 
                break;

        }
    }while(selection != EXIT_KEY);

    return(0);
}