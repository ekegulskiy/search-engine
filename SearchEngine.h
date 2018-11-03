/** 
 *  @file    SearchEngine.h
 *  @author  Eduard Kegulskiy
 *  @date    10/03/2018  
 *  @version 2.0
 *  
 *  @brief CSC 849, SearchEngine header file
 *
 *  @section DESCRIPTION
 *  
 *  Set of classes providing implementation of a small Search Engine
 * 
 *  NOTE: uses 3rd party stemmer by Krovetz: http://sourceforge.net/projects/lemur/files/lemur/KrovetzStemmer-
*   3.4/KrovetzStemmer-3.4.tar.gz
 *  
 */

#ifndef _SEARCH_ENGINE_H
#define _SEARCH_ENGINE_H

#include "KrovetzStemmer.hpp"
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <vector>
#include <string>
#include <cassert>
#include <map>
#include <sstream>
#include <set>
#include <algorithm>

using namespace std;
using namespace stem;

#define XML_TAG_DOC_OPEN    "<DOC"
#define XML_TAG_DOC_CLOSE   "</DOC>"

#define JSON_TAG_DOC_OPEN    "{\"context\":"
#define JSON_TAG_DOC_CLOSE   "\","
#define JSON_TAG_QUESTION_START "\"question\":"
#define JSON_TAG_QUESTION_STOP   "\"id\":"
#define JSON_TAG_ANSWER_START "\"text\":"
#define JSON_TAG_ANSWER_STOP   "\"}],"
#define JSON_TAG_ANSWER_STOP2   "\"},"

#define SPACE_STR            " "

typedef enum{
  DOCUMENT_TYPE_TEXT = 0,
  DOCUMENT_TYPE_IMAGE
}DOCUMENT_TYPE;

class Posting;
class TermInfo;
class ProximityQuery;
class Query;
typedef map<unsigned int, Posting> POSTING_LIST;
typedef map<string, TermInfo> TERMS_LIST;
typedef vector<unsigned long> POSITIONS_LIST;
typedef vector<ProximityQuery> PROXIMITY_QUERY_LIST;
typedef vector<Query> FREETEXT_QUERY_LIST;
typedef multimap<double, unsigned long> SCORES_LIST;

/**
 *  @brief Base class for a document in the collection.  
 */ 
class Document{
public:
    Document(){
    }
    virtual ~Document(){}

protected:
    DOCUMENT_TYPE m_type;
};

/**
 *  @brief Class for implementing text document.  
 */
class TextDocument : public Document{
public:
    explicit TextDocument(unsigned long id):
        m_id(id),
        m_length(0){}

    void appendToBody(string& text){
        m_body += text;

        if(text != SPACE_STR)
            m_length++;
    }
    
    void setBody(string& text){
        m_body = text;
    }
    
    void setTitle(string& title){
        m_title = title;
    }

    unsigned long length(){
        return m_length;
    }

protected:
    string         m_body;  // holds the full body of the document, unmodified
    string         m_title; // title of the document
    unsigned long  m_id;    // ID of the document
    unsigned long  m_length;
};

class Posting{
public:
    unsigned long docID;
    unsigned long tf;           // term frequency, i.e. how many times this term is present in the document
    POSITIONS_LIST positions;   // a list of all term positions in this document

    void print();
};

class TermInfo{
public:
    string term;                // term (i.e. index word)
    unsigned long df;           // document frequence, i.e. in how many documents in the collection this term is present
    POSTING_LIST  postings;     // list of postings (posting is created for each document where the term is present)

    void print(bool includePostings = true);
};

/**
 *  @brief Holds both original user query and tokenized list  
 */
class Query{
public: 
    explicit Query(string& queryText);
    vector<string>& terms();

protected: 
    string m_originalText;
    vector<string> m_terms;
};

/**
 *  @brief Holds 'proximity' queries, i.e. a 2-terms query where proximity distance
 *  is specified for them
 */
class ProximityQuery : public Query{
public:

    explicit ProximityQuery(string& queryText, unsigned long proximityWnd);

    unsigned long getProximityWnd(){return m_proximityWnd;}
private: 
    unsigned long m_proximityWnd;
};

/**
 *  @brief Singleton class used for tokenization and normalizations of free text
 */
class Tokenizer{
public: 
    static Tokenizer& singleton();

 /** 
 *   @brief  breaks free text into normlized tokens 
 *  
 *   @param  text free text (can be a user query or text from document)
 *   @return void
 */ 
    vector<string> tokenize(string& text);

protected:
 /** 
 *   @brief  applies stemming to the term.
 *           (uses Krovetz stemmer) 
 *  
 *   @param  term term to stem, modified upon return 
 *   @return void
 */   
    void stemTerm(string& term);

 /** 
 *   @brief  determines whether a word is a stop-word
 *  
 *   @param  word word to check 
 *   @return true if stop-word, false otherwise
 */     
    bool isStopWord(string& word);

private: 
    Tokenizer();
    static KrovetzStemmer m_stemmer;    // 3rd party stemmer
};

/**
 *  @brief Implements indexing of the documents including 
 *   tokenization, stemming, and normalization (i.e. lower-case conversion) 
 */
class Index{
public:
    
/** 
 *   @brief  adds new text into the index by performing  
 *           tokenization, stemming, and normalization  
 *  
 *   @param  text text to add, can contain multiple words 
 *   @param  docID document ID where text resides 
 *   @pos    position of the term in the document
 *   @return void
 */  
    void addText(string& text, unsigned long& docID, unsigned long& pos);

/** 
 *   @brief  prints index terms to the screen, including document frequency and posting lists 
 *  
 *   @return void
 */
    void print(bool includePostings = true);

/** 
 *   @brief  retrieves posting list for a give term in the index 
 *  
 *   @param  term term for which posting list is desired 
 *   @return pointer to POSTING_LIST
 */  
    const POSTING_LIST* getPostings(string term);

/** 
 *   @brief  retrieves term information 
 *  
 *   @param  term term for which info is desired 
 *   @return pointer to TermInfo
 */  
    const TermInfo* getTermInfo(string term);

/** 
 *   @brief  adds term into index if not already there. If already there, just adds a document ID to the posting list. 
 *  
 *   @param  term term being added 
 *   @param  docID id of the document where the term is from
 *   @pos    position of the term in the document
 *   @return void
 */
    void addTerm(string& term,unsigned long& docID, unsigned long& pos);

protected:

    TERMS_LIST m_terms;      // map of all terms in the index
};

/**
 *  @brief Implements search engine by building collection of the documents,
 *         creating an index for it, and evaluating queries
 */
class SearchEngine{
public:

/** 
 *   @brief  builds document collection from XML file containing multiple documents separated by <DOC> tags  
 *  
 *   @param  xmlFilePath path to XML file with document entries 
 *   @return void
 */  
    void buildFromFile(string xmlFilePath);

    void buildFromSquadData(string jsonFilePath, bool tokenizeCollection = false);


/** 
 *   @brief  prints index of the seach engine to the screen
 *  
 *   @return void
 */
    void printIndex(bool includePostings = true);

/** 
 *   @brief  performs boolean search against the document collection  
 *  
 *   @param  query a text query
 *   @return vector<unsigned long> list of documents mating the search criteria
 */
    vector<unsigned long> booleanSearch(string query);

 /** 
 *   @brief  performs ranked search against the document collection  
 *  
 *   @param  query a text query
 *   @return SCORES_LIST map of documents with non-zero ranking, sorted by rank value
 */   
    SCORES_LIST rankedSearch(string query);

protected:
/** 
 *   @brief implements intersection of two sets, based on algorithm from the assignment  
 *  
 *   @param  p1 set1 of posting lists
 *   @param  p2 set2 of posting lists
 *   @return intersection set
 */ 
    vector<unsigned long> intersect(const POSTING_LIST* p1, const POSTING_LIST* p2);

/** 
 *   @brief implements intersection of two sets, based on algorithm from the assignment  
 *  
 *   @param  v1 set1 of unique numbers
 *   @param  v2 set2 of unique numbers
 *   @return intersection set
 */     
    vector<unsigned long> intersect(vector<unsigned long> v1, vector<unsigned long> v2);

/** 
 *   @brief parses user query and builds 2 separate lists holding 'proximity' and 'free text' queries  
 *  
 *   @param  userQuestion user question (can be a mixed of 'free text' and 'proximity' query)
 *   @param  proxQueries list of 'proximity' queries, populated by the function
 *   @param  freeTextQueries list of 'free text' queries, populated by the function
 *   @return intersection set
 */     
    void buildQueries(string userQuestion, PROXIMITY_QUERY_LIST& proxQueries, FREETEXT_QUERY_LIST& freeTextQueries);

/** 
 *   @brief filters collection by proximity queries 
 *  
 *   @param  proxQueries list of 'proximity' queries to filter by
 *   @return a sub-set of document IDs from the whole collection matching all the proximity queries
 */
    vector<unsigned long> filterBy(PROXIMITY_QUERY_LIST& proxQueries);

/** 
 *   @brief intersects filtered set of documents with the set of the 'free text' query 
 *  
 *   @param  filterSet a set of documents to be intersected
 *   @param  freeTextQuery 'free text' query to use for intersection
 *   @return intersection of two sets
 */
    vector<unsigned long> intersectWithQuery(vector<unsigned long>& filterSet, Query& freeTextQuery);

/** 
 *   @brief detects whether 2 terms are located from each other with-in proximity window (order is important) 
 *  
 *   @param  p1 posting of term1
 *   @param  p2 posting of term2
 *   @param  proximityWnd proximity window
 *   @return true if there is a least one instance of correct proximity, otherwise - false.
 */
    bool findProximityPair(const Posting p1, const Posting p2, unsigned long proximityWnd );

/** 
 *   @brief scores a document based on the query. Usef TF.IDF alrogirthm for scoring
 *  
 *   @param  proxQueries list of 'proximity' queries extracted from original user query (see buildQueries() function)
 *   @param  freeTextQueries list of 'free text' queries extracted from original user query (see buildQueries() function)
 *   @param  docID document to score
 *   @param  score will hold the value when function returns
 *  
 *   @return true if score was calculated, false if not (i.e. this document does not contain any terms in the provided queries).
 */
    bool score(PROXIMITY_QUERY_LIST& proxQueries, FREETEXT_QUERY_LIST& freeTextQueries , unsigned long docID, double& score);

private:
    vector<Document*> m_collection;
    vector<unsigned long> m_collectionDocIDs;
    Index m_index;
};

#endif /*_SEARCH_ENGINE_H*/