/*
    a simple way to record the declaration of a Tranlastion Unit
*/
#include "data_encode.h"
#include <fstream>
#include <iomanip>
#include <set>
namespace SA{
RecordDecl::RecordDecl(const RecordDecl &copy) {
    TypeName = copy.TypeName;
    RecordType = copy.RecordType;
    Attrs = copy.Attrs;
    for(auto ptr : copy.Fields) {
        Fields.push_back(new Field(*ptr));
    }
}

RecordDecl::~RecordDecl() {
    while(!Fields.empty()) {
        auto ptr = Fields.back();
        Fields.pop_back();
        delete ptr;
    }
}

void RecordDecl::appendField(Field *field) {
    Fields.push_back(field);
}

void RecordDecl::dump(int indent) const {
    cout<<string(indent,' ');
    string type;
    switch(int(RecordType)) {
        case Struct: type = "Struct"; break;
        case Union: type = "Union"; break;
        case Typedef: type = "Typedef"; break;
    }
    cout << "TypeName : " << TypeName << " RecordType : " << type << " Size : " << size << endl;
    for(auto field : Fields) {
        field->dump(indent + 4);
    }
}

Field::Field(const Field &copy):AnonDecl(nullptr) {
    FieldName = copy.FieldName;
    TypeName = copy.TypeName;
    Qualifier = copy.Qualifier;
    if(copy.AnonDecl) {
        AnonDecl = new RecordDecl(*copy.AnonDecl);
    }
}

void Field::dump(int indent) const {
    cout<<string(indent,' ');
    if(AnonDecl) {
        cout<<"Field with Anon Decl FieldName : '"<<FieldName<<"'"<<endl;
        cout<<string(indent,' ');
        cout << "Size : " << size << " Offset : " << offset << endl;
        AnonDecl->dump(indent+4);
        return;
    } 
    cout<<"FieldName : "<<FieldName<<" TypeName : "<<TypeName<<endl;
    if(FieldName != "typedef_field") {
        cout<<string(indent,' ');
        cout << "Size : " << size << " Offset : " << offset << endl;
    }
}

jsonFile::jsonFile(const jsonFile &copy) {
    for(auto ptr : copy.Decls) {
        // Decls.push_back(new RecordDecl(*ptr));
        Decls.insert(new RecordDecl(*ptr));
    }
}

template<typename T>
void diff(const T &lhs, const T &rhs) {
    // cout<<"-----------------------diff---------------------"<<endl;
    // lhs.dump(0);
    // rhs.dump(0);
    // cout<<"-----------------------diff---------------------"<<endl;
}


/*相等函数*/

bool RecordDecl::operator==(const RecordDecl &lhs) const {
    if(TypeName != lhs.TypeName ||
        RecordType != lhs.RecordType ||
        Attrs.size() != lhs.Attrs.size()) {
        diff(*this,lhs);
        return false;
    }

    for(int i = 0; i < Attrs.size(); i++) {
        if(Attrs[i] != lhs.Attrs[i]) {
            diff(*this,lhs);
            return false;
        }
    }

    if(Fields.size() != lhs.Fields.size()) {
        diff(*this,lhs);
        return false;
    }

    for(int i = 0; i < Fields.size(); i++) {
        if(!(*Fields[i] == *lhs.Fields[i])) {
            // diff(*Fields[i],*lhs.Fields[i]);
            return false;
        }
    }
    return true;
}

bool RecordDecl::operator!=(const RecordDecl &lhs) const {
    return !(*this == lhs);
}

bool Field::operator==(const Field &lhs) const {
    if(FieldName != lhs.FieldName ||
        TypeName != lhs.TypeName ||
        Qualifier != lhs.Qualifier) {
        diff(*this,lhs);
        return false;
    }
    // 一方是anon 一方不是
    if((!!AnonDecl) ^ (!!lhs.AnonDecl)) {
        diff(*this,lhs);
        return false;
    }
    // 都是anon，进行进一步比较
    if(AnonDecl && !(*AnonDecl == *lhs.AnonDecl)) {
        // diff(*AnonDecl,*lhs.AnonDecl);
        return false;
    }
    return true;
}

bool Field::operator!=(const Field &lhs) const {
    return !(*this == lhs);
}


bool jsonFile::operator==(const jsonFile &lhs) const {
    if(Decls.size() != lhs.Decls.size()) {
        // diff(*this,lhs);
        return false;
    }
    // for(int i = 0; i < Decls.size(); i++) {
    //     if(!(*Decls[i] == *lhs.Decls[i])) {
    //         // diff(*Decls[i],*lhs.Decls[i]);
    //         return false;
    //     }
    // }
    auto it1 = Decls.begin();
    auto it2 = lhs.Decls.begin();
    for(;it1 != Decls.end(); it1++, it2++){
        if(!(**it1 == **it2)) {
            return false;
        }
    }
    return true;
}

bool jsonFile::operator!=(const jsonFile &lhs) const {
    return !(*this == lhs);
}

using prr = std::pair<RecordDecl*,RecordDecl*>;

/*
[
{
    "DiffType" : TypeUnmatch/NameUnmatch/....
    "file1" : RecordDecl
    "file2" : RecoudDecl
},
{
.......
}
]
*/

static void dumpPSS(std::set<prr> &set,std::string name) {
    int i = 0;
    json j;
    for(auto [a,b] : set) {
        j[i]["DiffType"] = name;
        j[i]["file1"] = *a;
        j[i++]["file2"] = *b;
    }
    std::ofstream of("./"+name);
    of << setw(4) << j;
}

bool jsonFile::DiffJsonFile(jsonFile &File1, jsonFile &File2) {
    std::set<RecordDecl *> Unique1,Unique2;
    //分别表示type相同name不同；type不同；成员数量不同；顺序不同
    std::set<prr> NameUnmatch,TypeUnmatch,MemberSizeUnmatch,SeqUnmatch;
    for(auto *Decl1 : File1.Decls) {
        bool matched = false;
        for(auto *Decl2 : File2.Decls) {
            if(Decl1->TypeName == Decl2->TypeName) {
                matched = true;
                if(*Decl1 != *Decl2) {
                    if(Decl1->Fields.size() != Decl2->Fields.size()) {
                        MemberSizeUnmatch.insert({Decl1,Decl2});
                        continue;
                    }
                    auto &fields1 = Decl1->Fields;
                    auto &fields2 = Decl2->Fields;
                    auto f1 = fields1.begin();
                    auto f2 = fields2.begin();
                    int flag = 0;
                    for(;f1 != fields1.end(); f1++,f2++) {
                        if(((*f1)->isAnon() ^ (*f2)->isAnon())) {
                            TypeUnmatch.insert({Decl1,Decl2});
                            flag = 1;
                            break;
                        }
                        if((*f1)->isAnon() && (*f2)->isAnon()) {
                            if(*(*f1)->getAnonDecl() != *(*f2)->getAnonDecl()) {
                                TypeUnmatch.insert({Decl1,Decl2});
                                flag = 1;
                                break;
                            }
                        } else {
                            if((*f1)->getTypeName() != (*f2)->getTypeName()) {
                                TypeUnmatch.insert({Decl1,Decl2});
                                flag = 1;
                                break;
                            }
                        }

                    }
                    if(flag) continue;

                    f1 = fields1.begin();
                    f2 = fields2.begin();
                    flag = 0;
                    for(;f1 != fields1.end(); f1++,f2++) {
                        if((*f1)->getFieldName() != (*f2)->getFieldName()) {
                            NameUnmatch.insert({Decl1,Decl2});
                            flag = 1;
                            break;
                        }
                    }
                    if(flag) continue;

                    cout << "==================diff==================\n";
                    Decl1->dump(0);
                    cout << "----------------------------------\n";
                    Decl2->dump(0);
                    cout << "==================diff==================\n";
                }
            }
        }
        if(!matched) {
            Unique1.insert(Decl1);
        }
    }

    cout<<NameUnmatch.size() << " "<<TypeUnmatch.size() << " "<<MemberSizeUnmatch.size() << "\n";
    cout << "#############Name Unmatch#############\n";
    for(auto [RD1,RD2] : NameUnmatch) {
        cout << "=======================================\n";
        RD1->dump(0);
        cout << "---------------------------------------\n";
        RD2->dump(0);
        cout << "=======================================\n";
    }
    cout << "#############Name Unmatch#############\n";

    cout << "#############Type Unmatch#############\n";
    for(auto [RD1,RD2] : TypeUnmatch) {
        cout << "=======================================\n";
        RD1->dump(0);
        cout << "---------------------------------------\n";
        RD2->dump(0);
        cout << "=======================================\n";
    }
    cout << "#############Type Unmatch#############\n";

    cout << "#############Member Size Unmatch#############\n";
    for(auto [RD1,RD2] : MemberSizeUnmatch) {
        cout << "=======================================\n";
        RD1->dump(0);
        cout << "---------------------------------------\n";
        RD2->dump(0);
        cout << "=======================================\n";
    }
    cout << "#############Member Size Unmatch#############\n";

    cout << "#############unique in File1#############\n";
    for(auto *Decl : Unique1) {
        cout << "------------------------------------\n";
        Decl->dump(0);
        cout << "------------------------------------\n";
    }
    cout << "#############unique in File1#############\n";

    for(auto *Decl2 : File2.Decls) {
        bool matched = false;
        for(auto *Decl1 : File1.Decls) {
            if(Decl1->TypeName == Decl2->TypeName) {
                matched = true;
                break;
            }
        }
        if(!matched) {
            Unique2.insert(Decl2);
        }
    }
    cout << "#############unique in File2#############\n";
    for(auto *Decl : Unique2) {
        cout << "------------------------------------\n";
        Decl->dump(0);
        cout << "------------------------------------\n";
    }
    cout << "--------------unique in File2----------------\n";
    dumpPSS(NameUnmatch, "NameUnmatch");
    dumpPSS(TypeUnmatch, "TypeUnmatch");
    dumpPSS(MemberSizeUnmatch, "MemberSizeUnmatch");
    dumpPSS(SeqUnmatch, "SeqUnmatch");
    return true;
}

void to_json(json& j, const jsonFile& File) {
    int index = 0;
    for(auto decl : File.getDecls()) {
        to_json(j[index++],*decl);
    }
}

void from_json(const json& j, jsonFile& File) {
    for(int i = 0; i < j.size() ; i++) {
        RecordDecl *decl = new RecordDecl();
        from_json(j[i],*decl);
        File.addDecls(decl);
    }
}

void to_json(json& j, const Field& Field) {
    j["FieldName"] = Field.getFieldName();
    j["TypeName"] = Field.getTypeName();
    j["Size"] = Field.size;
    j["Offset"] = Field.offset;
    // j["Qualifier"] = Field.getQualifier();
    if(Field.isAnon()) {
        to_json(j["AnonDecl"],*Field.getAnonDecl());
    }
}

void from_json(const json& j, Field& Field) {
    j.at("FieldName").get_to(Field.FieldName);
    j.at("TypeName").get_to(Field.TypeName);
    Field.size = j["Size"];
    Field.offset = j["Offset"];
    // j.at("Qualifier").get_to(Field.Qualifier);
    if(Field.isAnon()) {
        Field.AnonDecl = new RecordDecl();
        from_json(j.at("AnonDecl"),*Field.AnonDecl);
    }
}

void to_json(json& j, const RecordDecl& RecordDecl){
    j["TypeName"] = RecordDecl.TypeName;
    j["RecordType"] = int(RecordDecl.RecordType);
    j["Size"] = RecordDecl.size;
    for(auto attr : RecordDecl.Attrs) {
        j["Attrs"].push_back(attr);
    }
    int index = 0;
    for(auto field : RecordDecl.Fields) {
        to_json(j["Fields"][index++],*field);
    }
}

void from_json(const json& j, RecordDecl& RecordDecl){
    RecordDecl.TypeName = j["TypeName"];
    RecordDecl.RecordType = j["RecordType"];
    RecordDecl.size = j["Size"];
    if(j.count("Attrs")) {
        for(int i = 0; i < j["Attrs"].size(); i++) {
            string temp;
            j.at("Attrs").at(i).get_to(temp);
            RecordDecl.Attrs.push_back(temp);
        }
    }
    if(j.count("Fields")) {
        for(int i = 0; i < j["Fields"].size(); i++) {
            RecordDecl.Fields.push_back(new Field());
            from_json(j["Fields"][i],*RecordDecl.Fields.back());
        }
    }
}


};