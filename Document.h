#ifndef DOCUMENT_H
#define DOCUMENT_H
#include <string>

class Document { //Hacer Key / nodos(lista enlazada) que guarde los documentos
public:
    std::string url; // Url del documento
    Document *next = nullptr;
    int frecuencia = 1; // Frecuencia iniciada en 1
    
    Document(std::string _url) : url(_url), next(nullptr), frecuencia(1) {}

    void setNext(Document* _next) {
        next = _next;
    }

};

class listaString {
public:
    std::string value;
    listaString *next = nullptr;

    listaString(std::string _value) : value(_value), next(nullptr) {}

};

#endif // DOCUMENT_H
