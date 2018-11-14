/** 
 *  @file    SearchEngine.cpp
 *  @author  Eduard Kegulskiy
 *  @date    9/10/2018  
 *  @version 1.0 
 *  
 *  @brief CSC 849, SearchEngine implementation
 *
 *  @section DESCRIPTION
 *  
 *  Set of classes providing implementation of inverted index 
 *  and boolean query evaluation.
 *  
 */

#include "SearchEngine.h"
#include <math.h>

KrovetzStemmer Tokenizer::m_stemmer;

Tokenizer& Tokenizer::singleton(){

    static Tokenizer singletonObj; // created only once
    return singletonObj;
}

vector<string> Tokenizer::tokenize(string& text){
    string token;
    vector<string> tokens;

    for(unsigned int i = 0; i < text.length(); i++){
        char c = text[i];

        if(isalpha(c) || isdigit(c)){
            token += tolower(c); // convert to lower case as we are building the token
        }
        else if( token != "")
        {
            // we got a word boundary - let's add current word and prepare for next one
            if(!isStopWord(token)){
                stemTerm(token);
                tokens.push_back(token);
            }
            token = "";
        }
    }

    if(token != "" && !isStopWord(token)){
        stemTerm(token);
#if 0
        if( token == "unives")
        {
            int x =0;
            x =9;
        }
#endif
        tokens.push_back(token); // add last token
    }
    return tokens;
}

Tokenizer::Tokenizer(){

}

void Tokenizer::stemTerm(string& term){       
    if(term.length() <= KrovetzStemmer::MAX_WORD_LENGTH){
        char thestem[80];
        char word[80];
        int ret = 0;

        strcpy(word, term.c_str());
        ret = m_stemmer.kstem_stem_tobuffer(word, thestem);

        if(ret > 0){
            term = thestem; // successful stemming
        }
    }
}

bool Tokenizer::isStopWord(string& word){
    if( word == "the" || word == "is" || word == "at" ||
        word == "of" || word =="on" || word == "and" ||
        word == "a" ){
        return true;
    }
    else{
        return false;
    }
}

Query::Query(string& queryText): 
        m_originalText(queryText){

    m_terms = Tokenizer::singleton().tokenize(m_originalText);
}


vector<string>& Query::terms(){
    return m_terms;
}


ProximityQuery::ProximityQuery(string& queryText, unsigned long proximityWnd): 
                        Query(queryText),
                        m_proximityWnd(proximityWnd)
{

}

void Posting::print(){
    cout << "[" << docID << "," << tf << ": ";

    POSITIONS_LIST::iterator it = positions.begin();
    if(it != positions.end()){
        do{
            cout << (*it);
            it++;
            if(it != positions.end())
                cout << ",";
        }while(it != positions.end());
    }

    cout << "]";
}

void TermInfo::print(bool includePostings){
    cout << "[" << term << ": " << postings.size() << "]";

    if(includePostings){
        unsigned int index = 0;
        cout << "->";
        for(POSTING_LIST::iterator pit = postings.begin(); pit != postings.end(); pit++){
            Posting& posting = (*pit).second;
            posting.print();
            if(index++ < postings.size() - 1)
                cout << ",";
        }
    }
    cout << endl;
}

void Index::addTerm(string& term, unsigned long& docID, unsigned long& pos){
    // check if this token already exists
    TERMS_LIST::iterator it = m_terms.find(term);
    TermInfo * pTermInfo = NULL;

    if(it != m_terms.end()){
        pTermInfo = &(*it).second;
    }
    else{
        TermInfo termInfo;
        m_terms[term] = termInfo;  // insert new list
        pTermInfo = &m_terms[term];

    }

    assert(pTermInfo);
    pTermInfo->term = term;
    pTermInfo->postings[docID].docID = docID;
    pTermInfo->postings[docID].positions.push_back(pos);
    pTermInfo->postings[docID].tf++;
    pTermInfo->df = pTermInfo->postings.size(); // update df
}

void Index::addText(string& text, unsigned long& docID, unsigned long& pos){
    vector<string> tokens = Tokenizer::singleton().tokenize(text);
 
    for( unsigned int i = 0; i < tokens.size(); i++){
        addTerm(tokens[i], docID, pos);

        if(i > 0)
            pos++; // increment position for each new token
    }
}

void Index::print(bool includePostings){
 //   cout << "Index contents: " << endl;

    for(TERMS_LIST::iterator it = m_terms.begin(); it != m_terms.end(); it++){
        TermInfo& termInfo = (*it).second;
        termInfo.print(includePostings);
    }
}

const POSTING_LIST* Index::getPostings(string term){
    TERMS_LIST::iterator it = m_terms.find(term);
 
    if(it != m_terms.end()){
        return &((*it).second.postings);
    }
    else{
        return NULL;
    }
}

const TermInfo* Index::getTermInfo(string term){
    TERMS_LIST::iterator it = m_terms.find(term);
 
    if(it != m_terms.end()){
        return &((*it).second);
    }
    else{
        return NULL;
    }
}


void SearchEngine::printIndex(bool includePostings){
    m_index.print(includePostings);
}

void SearchEngine::buildFromFile(string xmlFilePath){
    ifstream inFile;
    string token;
    unsigned long docID = 0;
    unsigned long termPos = 0;
    TextDocument* pTextDoc = NULL;
    string space = SPACE_STR;
    string closing_bracket;

    inFile.open(xmlFilePath.c_str());
    if (!inFile) {
        cout << "Unable to open file";
        exit(1); // terminate with error
    }
    
    while (inFile >> token) {
    
        if(token == XML_TAG_DOC_OPEN){
            inFile >> docID;
            assert(docID != 0);
  
            inFile >> closing_bracket;
            assert (closing_bracket == ">");

            pTextDoc = new TextDocument(docID);
        }
        else if( token == XML_TAG_DOC_CLOSE){
            m_collectionDocIDs.push_back(docID);
            m_collection.push_back(pTextDoc);

            docID = 0; // clear the ID to ensure next document will properly contain an ID.
            pTextDoc = NULL;
            termPos = 0;
        }
        else if (docID != 0){
            termPos++; // increment by one to get position of this new term
            // we are reading in the document, token by token
            pTextDoc->appendToBody(token);
            pTextDoc->appendToBody(space);
            m_index.addText(token, docID, termPos);
        }
    }
    
    inFile.close();  
}

void SearchEngine::buildFromSquadData(string jsonFilePath, bool tokenizeCollection){
    ifstream inFile;
    string token;
    unsigned long docID = 0;
    unsigned long termPos = 0;
    TextDocument* pTextDoc = NULL;
    string closing_bracket;
    size_t docBegin = 0, pos;
    unsigned long biggestDocSize = 0;
    string space = SPACE_STR;
    ofstream tokenizedDocsFile;
    bool processingQuestion = false, processingAnswer =false;
    unsigned long posUnused = 0;
    unsigned long fakeDocID = 0;

    if(tokenizeCollection)
    {
        string tokenizedDocPath = jsonFilePath + "tokenized";
        tokenizedDocsFile.open(tokenizedDocPath);
    
        if (!tokenizedDocsFile) {
            cout << "Unable to open file: " + tokenizedDocPath;
            exit(1); // terminate with error
        }
    }

    inFile.open(jsonFilePath.c_str());
    if (!inFile) {
        cout << "Unable to open file";
        exit(1); // terminate with error
    }
    
    string line;
    vector<string> tokens;

    while( getline(inFile, line) ) {

        stringstream singleLineText( line );

        while (singleLineText >> token) {

#if 0
            pos = token.find("unives");
            if(pos != std::string::npos)
            {
                int x = 0;
                x = 9;
            }
#endif

            docBegin = token.find(JSON_TAG_DOC_OPEN);
            if(docBegin == 0 || docBegin == 1){
                docID++;
    
                pTextDoc = new TextDocument(docID);

                if(tokenizeCollection)
                    tokenizedDocsFile << token << " \"";
            }
            else if( pTextDoc && token.length() >= 2 &&
                    token.rfind(JSON_TAG_DOC_CLOSE) == token.length()-2 &&
                    token[token.length()-3] != '\\'){
                
                termPos++; // increment by one to get position of this new term
                // we are reading in the document, token by token
                pTextDoc->appendToBody(token);
                pTextDoc->appendToBody(space);
                m_index.addText(token, docID, termPos);
                
                m_collectionDocIDs.push_back(docID);
                m_collection.push_back(pTextDoc);

                if(biggestDocSize < pTextDoc->length())
                    biggestDocSize = pTextDoc->length();

                pTextDoc = NULL;
                termPos = 0;

                if(tokenizeCollection){
                    tokens = Tokenizer::singleton().tokenize(token);
                    for(int i = 0; i < tokens.size(); i++)
                        tokenizedDocsFile << tokens[i] << " ";  

                    if(token.find(".") != std::string::npos)
                        tokenizedDocsFile << ".";
                    tokenizedDocsFile << JSON_TAG_DOC_CLOSE << " ";
                }
            }
            else if (pTextDoc){
                termPos++; // increment by one to get position of this new term
                // we are reading in the document, token by token
                pTextDoc->appendToBody(token);
                pTextDoc->appendToBody(space);
                m_index.addText(token, docID, termPos);

                if(tokenizeCollection)
                {
                    tokens = Tokenizer::singleton().tokenize(token);
                    for(int i = 0; i < tokens.size(); i++)
                        tokenizedDocsFile << tokens[i] << " ";

                    if(token.find(".") != std::string::npos)
                        tokenizedDocsFile << ".";
                }
            }
            else{
                if(tokenizeCollection)
                {
                    pos = token.find(JSON_TAG_QUESTION_START);

                    if(pos != std::string::npos){
                        processingQuestion = true;
                        tokenizedDocsFile << token << " \"";
                    }
                    else if( processingQuestion &&
                             token.find(JSON_TAG_QUESTION_STOP) != std::string::npos){
                        processingQuestion = false;

                        tokenizedDocsFile << "\", " << token;
                    }
                    else if(processingQuestion){
                        tokens = Tokenizer::singleton().tokenize(token);
                        for(int i = 0; i < tokens.size(); i++)
                        {
                            tokenizedDocsFile << tokens[i] << " ";
                            posUnused = 0;
                            fakeDocID = 0;
                            m_index.addTerm(tokens[i], fakeDocID, posUnused);
                        }
                    }
                    else if(token.find(JSON_TAG_ANSWER_START) != std::string::npos){
                        processingAnswer = true;
                        tokenizedDocsFile << token << " \"";
                    }
                    else if( processingAnswer && token.length() >= 4 &&
                             token.rfind(JSON_TAG_ANSWER_STOP) == token.length()-4){
                        processingAnswer = false;

                        tokens = Tokenizer::singleton().tokenize(token);
                        for(int i = 0; i < tokens.size(); i++)
                        {
                            posUnused = 0;
                            fakeDocID = 0;
                            m_index.addTerm(tokens[i], fakeDocID, posUnused);    
                            tokenizedDocsFile << tokens[i] << " ";  
                        }
                        tokenizedDocsFile << JSON_TAG_ANSWER_STOP << " ";
                    }
                    else if( processingAnswer && token.length() >= 3 &&
                             token.rfind(JSON_TAG_ANSWER_STOP2) == token.length()-3){
                        processingAnswer = false;

                        tokens = Tokenizer::singleton().tokenize(token);
                        for(int i = 0; i < tokens.size(); i++)
                        {
                            posUnused = 0;
                            fakeDocID = 0;
                            m_index.addTerm(tokens[i], fakeDocID, posUnused);     
                            tokenizedDocsFile << tokens[i] << " ";  
                        }
                        tokenizedDocsFile << JSON_TAG_ANSWER_STOP2 << " ";
                    }                    
                    else if(processingAnswer){
                        tokens = Tokenizer::singleton().tokenize(token);
                        for(int i = 0; i < tokens.size(); i++)
                        {
                            posUnused = 0;
                            fakeDocID = 0;
                            m_index.addTerm(tokens[i], fakeDocID, posUnused);  
                            tokenizedDocsFile << tokens[i] << " ";
                        }
                    }    
                    else
                        tokenizedDocsFile << token << " ";
                }
            }
        }

        if(tokenizeCollection)
            tokenizedDocsFile << endl;
    }
    inFile.close();  
    if(tokenizeCollection)
        tokenizedDocsFile.close();

   // cout << "Biggest doc in collection contains " << biggestDocSize << " words" << endl;
}

vector<unsigned long> SearchEngine::intersect(vector<unsigned long> v1, vector<unsigned long> v2){
    vector<unsigned long> intersection;

    vector<unsigned long>::const_iterator v1_it = v1.begin();
    vector<unsigned long>::const_iterator v2_it = v2.begin();

    while(v1_it != v1.end() && v2_it != v2.end()){
        if( (*v1_it) == (*v2_it)){
            // docID matched, add to the answer
            intersection.push_back(*v1_it);
            v1_it++;
            v2_it++;
        }
        else if(*v1_it < *v2_it){
            v1_it++;
        }
        else{
            v2_it++;
        }
    }

    return intersection;
}

vector<unsigned long> SearchEngine::intersect(const POSTING_LIST* p1, const POSTING_LIST* p2){
    vector<unsigned long> answer;

    if(p1 && p2){
        POSTING_LIST::const_iterator p1_it = p1->begin();
        POSTING_LIST::const_iterator p2_it = p2->begin();

        while(p1_it != p1->end() && p2_it != p2->end()){
            if( (*p1_it).first == (*p2_it).first){
                // docID matched, add to the answer
                answer.push_back((*p1_it).first);
                p1_it++;
                p2_it++;
            }
            else if((*p1_it).first < (*p2_it).first){
                p1_it++;
            }
            else{
                p2_it++;
            }
        }
    }

    return answer;
}

void SearchEngine::buildQueries(string userQuestion, PROXIMITY_QUERY_LIST& proxQueries, FREETEXT_QUERY_LIST& freeTextQueries){
    string curQuery;
    unsigned long proxWnd = 0;

    for(unsigned int i=0; i < userQuestion.length(); i++){       
        if(i < userQuestion.length()-1 && isdigit(userQuestion[i]) && userQuestion[i+1] == '('){
            // beginning of proximity query
            
            proxWnd = userQuestion[i] - '0'; // convert ASCII to digit

            if(curQuery != ""){
                Query freeTextQuery(curQuery);
                if(freeTextQuery.terms().size() > 0)
                    freeTextQueries.push_back(freeTextQuery);
                curQuery = "";
            }
            i++; // skip the bracket
        }
        else if(userQuestion[i] == ')'){
            // end of proximity query
            if(curQuery != ""){
                ProximityQuery proxQ(curQuery, proxWnd);
                if(proxQ.terms().size() > 0)
                    proxQueries.push_back(proxQ);
                curQuery = "";
            }
        }
        else
            curQuery += userQuestion[i];
    }

    if(curQuery != ""){
        freeTextQueries.push_back(Query(curQuery));
    }   
}

bool SearchEngine::findProximityPair(const Posting p1, const Posting p2, unsigned long proximityWnd ){
    for(unsigned int i = 0; i < p1.positions.size(); i++){
        long pos1 = p1.positions[i];

        for(unsigned int k = 0; k < p2.positions.size(); k++){
            long pos2 = p2.positions[k];

            if(pos2 - pos1 > 0  && pos2 - pos1 <= (proximityWnd+1)) // adding 1 to proximity window since distance is represented by the difference in indexes 
                return true;
        }
    }
    return false;
}

vector<unsigned long> SearchEngine::filterBy(PROXIMITY_QUERY_LIST& proxQueries){
    vector<unsigned long> combinedResults;
    vector<unsigned long> curQueryResult;

    for(int i=0; i < proxQueries.size(); i++){
        vector<string> terms = proxQueries[i].terms();

        const POSTING_LIST* pTerm1List = m_index.getPostings(terms[0]);
        const POSTING_LIST* pTerm2List = m_index.getPostings(terms[1]);
        vector<unsigned long> termsIntercectionSet = intersect(pTerm1List, pTerm2List);    

        // check positioning
        curQueryResult.clear();
        for(unsigned long y = 0; y < termsIntercectionSet.size(); y++){
            unsigned long docID = termsIntercectionSet[y];
            const Posting p1 = pTerm1List->find(docID)->second;
            const Posting p2 = pTerm2List->find(docID)->second;

            if(findProximityPair(p1,p2, proxQueries[i].getProximityWnd()))
                curQueryResult.push_back(docID);
        }

        if(i == 0)
            combinedResults = curQueryResult;

        combinedResults = intersect(combinedResults, curQueryResult);
    }

    return combinedResults;
}

vector<unsigned long> SearchEngine::intersectWithQuery(vector<unsigned long>& filterSet, Query& freeTextQuery){
    // execute intersection algorithm
    vector<unsigned long> intersection;
    vector<string> terms =freeTextQuery.terms();

    for(int i=0; i < terms.size(); i++){
        const POSTING_LIST* pTermList = m_index.getPostings(terms[i]);

        vector<unsigned long> curTermList = intersect(pTermList, pTermList);
        if(i == 0)
            intersection = curTermList;
        else    
            intersection = intersect(intersection, curTermList);
    }

    if(filterSet.size() > 0)
        intersection = intersect(filterSet, intersection);

    return intersection;
}

vector<unsigned long> SearchEngine::booleanSearch(string query)
{
    vector<unsigned long> searchResultSet;
    // extract proximity queries and free-text queries into separate lists
    PROXIMITY_QUERY_LIST proxQueries;
    FREETEXT_QUERY_LIST freeTextQueries;
    buildQueries(query, proxQueries, freeTextQueries);

    if(proxQueries.size() > 0){
        // filter search by proximity queries if any
        vector<unsigned long> filteredSet = filterBy(proxQueries);

        if(filteredSet.size() > 0){
            // intersect filteredSet with search results of free text queries
            for(unsigned int i=0; i < freeTextQueries.size(); i++){          
                filteredSet = intersectWithQuery(filteredSet, freeTextQueries[i]);
            }
        }

        searchResultSet = filteredSet;
    }
    else{
        // just intersect search results of free text queries, with empty "filtere set" 
        for(unsigned int i=0; i < freeTextQueries.size(); i++){          
            searchResultSet = intersectWithQuery(searchResultSet, freeTextQueries[i]);
        }
    }

    return searchResultSet;
}


bool SearchEngine::score(PROXIMITY_QUERY_LIST& proxQueries, FREETEXT_QUERY_LIST& freeTextQueries , unsigned long docID, double& score){
    score = 0.0;
    bool atLeastOneTermInDoc = false;

    vector<string> allTerms;

    // put all terms into one single list
    for(unsigned long i=0; i < proxQueries.size(); i++){
        vector<string>& curQueryTerms = proxQueries[i].terms();
        allTerms.insert( allTerms.end(), curQueryTerms.begin(), curQueryTerms.end() );
    }

    for(unsigned long i=0; i < freeTextQueries.size(); i++){
        vector<string>& curQueryTerms = freeTextQueries[i].terms();
        allTerms.insert( allTerms.end(), curQueryTerms.begin(), curQueryTerms.end() );
    }

    // sum up weights of each term present in the doc
    for(unsigned long i=0; i < allTerms.size(); i++){
        const POSTING_LIST* pTerm1List = m_index.getPostings(allTerms[i]);
        const TermInfo* pTermInfo = m_index.getTermInfo(allTerms[i]);

        if(pTerm1List &&pTermInfo){
            POSTING_LIST::const_iterator it = pTerm1List->find(docID);
            if(it != pTerm1List->end()){
                const Posting& posting = it->second;

                double N = static_cast<double>(m_collectionDocIDs.size());
                double df = static_cast<double>(pTermInfo->df);
                double tf = static_cast<double>(posting.tf);

                double w = (1 + log2(tf))*(log2(N/df));
                score += w;

                atLeastOneTermInDoc = true;
            }
        }
    }
    return atLeastOneTermInDoc;
}

SCORES_LIST SearchEngine::rankedSearch(string query)
{
    vector<unsigned long> searchSet;
    // extract proximity queries and free-text queries into separate lists
    PROXIMITY_QUERY_LIST proxQueries;
    FREETEXT_QUERY_LIST freeTextQueries;
    buildQueries(query, proxQueries, freeTextQueries);

    if(proxQueries.size() > 0){
        // filter search by proximity queries if any
        searchSet = filterBy(proxQueries);
    }
    else{
        searchSet = m_collectionDocIDs;
    }

    SCORES_LIST scoresSet;

    for(unsigned long i=0; i < searchSet.size(); i++){
        double docScore;
        unsigned long docID;

        if(score(proxQueries, freeTextQueries, searchSet[i], docScore)){
            docID = searchSet[i];
            scoresSet.insert(pair<double,unsigned long>(docScore, docID));
        }
    }

    return scoresSet;
}