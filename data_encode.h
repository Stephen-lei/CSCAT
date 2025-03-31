#pragma once

/*
rule to generate json file:
jsonfile    ->  { [ RecordDecl ( ,RecordDecl ) ] }
RecordDecl  ->  { TypeName : str,
                  RecordType : "Union" or "Struct",
                  Fields : [ Field ( ,Field ) ]
                }       
Field       ->  { FieldName : str,
                  TypeName : str,
                  Qualifier ： str,
                  Attr : [ str ( ,str ) ]，
                  (AnonDecl : RecordDecl)
                }
FieldName and TypeName of Field may be equal to ""
When TypeName equal to "", it means that there is an anonymous declaration like:
        struct T {
            struct {
                int a;
            };
            int b;
        };
so the AnonDecl field is needed
*/

#include <iostream>
#include <set>
#include <string>
#include <vector>
#include "nlohmann/json.hpp"
// #include "include/nlohmann/json.hpp"

using namespace std;
using json = nlohmann::json;

namespace SA {
class Field;

enum RecordType_t {Union, Struct, Enum, Typedef};

class RecordDecl {
public:
friend void to_json(json& j, const RecordDecl& RecordDecl);
friend void from_json(const json& j, RecordDecl& RecordDecl);
friend class jsonFile;
public:
    RecordDecl():size(-1){};
    RecordDecl(string TypeName,RecordType_t RecordType):
        TypeName(TypeName),RecordType(RecordType),Attrs(),size(-1) {}
    RecordDecl(const RecordDecl &copy);
    ~RecordDecl();

    bool isUnion() { return RecordType == Union; }
    bool isStruct() { return RecordType == Struct; }
    bool isEnum() { return RecordType == Enum; }
    bool isAnon() { return TypeName.find('#') != string::npos; }

    void setTypeName (string TypeName) { this->TypeName = TypeName; }
    void setRecordType (RecordType_t RecordType) { this->RecordType = RecordType; }
    void setSize(int size) { this->size = size; }
    void appendAttr(string Attr) { Attrs.push_back(Attr); }
    void appendField (Field *field);
    void dump (int indent) const;

    bool isEmpty() {return Fields.empty();}
    std::string getTypeName() {return TypeName;}

    bool operator==(const RecordDecl &lhs) const;
    bool operator!=(const RecordDecl &lhs) const;

private:
    string TypeName;
    RecordType_t RecordType;
    vector<string> Attrs;
    vector<Field *> Fields;
    int size;
};

class Field {
public:
friend void to_json(json& j, const Field& Field);
friend void from_json(const json& j, Field& Field);
public:
    Field():size(-1),offset(-1) {};
    Field(string FieldName,string TypeName,string Qualifier):
        FieldName(FieldName),TypeName(TypeName),Qualifier(Qualifier),AnonDecl(nullptr),size(-1),offset(-1) {}
    Field(const Field &copy);
    ~Field() {
        delete AnonDecl;
    }

    bool isAnon() const {return TypeName.find('#') != string::npos;};

    void setFieldName (string FieldName) { this->FieldName = FieldName; }
    void setTypeName (string TypeName) { this->TypeName = TypeName; }
    void setQualifier (string Qualifier) { this->Qualifier = Qualifier; }
    void setAnonDecl (RecordDecl *AnonDecl) { this->AnonDecl = AnonDecl; }
    void setSize(int size) { this->size = size; }
    void setOffset(int offset) { this->offset = offset; }

    string getFieldName() const { return FieldName; }
    string getTypeName() const { return TypeName; }
    string getQualifier() const { return Qualifier; }
    const RecordDecl *getAnonDecl() const { return AnonDecl; }

    void dump (int indent) const;

    bool operator==(const Field &lhs) const;
    bool operator!=(const Field &lhs) const;

private:
    string FieldName;
    string TypeName;
    string Qualifier;
    RecordDecl *AnonDecl;
    int size;
    int offset;
};



class jsonFile {
public:
friend void to_json(json& j, const jsonFile& File);
friend void from_json(const json& j, jsonFile& File);

class RDCompare {
public:
    bool operator()(const RecordDecl* lhs, const RecordDecl* rhs) const {
        return lhs->TypeName < rhs->TypeName;
    }
};

public:

    jsonFile() = default;
    jsonFile(const jsonFile &copy);
    ~jsonFile() {
        // while(!Decls.empty()) {
        //     auto ptr = Decls.back();
        //     Decls.pop_back();
        //     delete ptr;
        // }
        for(auto ptr : Decls) {
            delete ptr;
        }
    }
    const multiset<RecordDecl*,RDCompare> &getDecls() const { return Decls; }
    void addDecls(RecordDecl *decl) {
        // Decls.push_back(decl); 
        Decls.insert(decl);
    }
    void dumpJsonFile() {
        int count = 0;
        cout<<"-------------dump json file------------"<<endl;
        for(auto decl : Decls) {
            // std::cout << "count: " << count++ << " ";
            decl->dump(0);
        }
    };

    bool operator==(const jsonFile &rhs) const;
    bool operator!=(const jsonFile &rhs) const;
    static bool DiffJsonFile(jsonFile &File1,jsonFile &File2);

private:
    // std::set<RecordDecl*,RDCompare> Decls;
    std::multiset<RecordDecl*,RDCompare> Decls;
};

// void to_json(json& j, const jsonFile& File);
// void from_json(const json& j, jsonFile& File);
// void to_json(json& j, const Field& Field);
// void from_json(const json& j, Field& Field);
// void to_json(json& j, const RecordDecl& RecordDecl);
// void from_json(const json& j, RecordDecl& RecorDecl);
}; // End Of Namespace