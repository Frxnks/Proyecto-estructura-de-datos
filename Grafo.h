#pragma once
#include <map>
#include <vector>
#include <string>
#include <fstream>
#include <iostream>

class Grafo {
public:
    // Lista de adyacencia: nodo origen -> vector de (nodo destino, peso)
    std::map<std::string, std::vector<std::pair<std::string, int>>> adj;

    // Agrega una arista no dirigida (ambos sentidos)
    void agregarAristaNoDirigida(const std::string& a, const std::string& b) {
        agregarAristaSimple(a, b);
        agregarAristaSimple(b, a);
    }

private:
    // Agrega una arista simple de a hacia b o incrementa su peso si ya existe
    void agregarAristaSimple(const std::string& origen, const std::string& destino) {
        for (auto& par : adj[origen]) {
            if (par.first == destino) {
                par.second++; // Incrementa el peso si ya existe
                return;
            }
        }
        adj[origen].push_back({destino, 1}); // Nueva arista con peso 1
    }

public:
    // Guarda el grafo en un archivo como lista de adyacencia
    void guardarEnArchivo(const std::string& filename) const {
        std::ofstream out(filename);
        if (!out.is_open()) {
            std::cerr << "No se pudo crear " << filename << std::endl;
            return;
        }
        for (const auto& nodo : adj) {
            out << "[" << nodo.first << "] -> ";
            for (const auto& vecino : nodo.second) {
                out << "[" << vecino.first << "~> peso: " << vecino.second << "] ";
            }
            out << std::endl;
        }
        out.close();
    }

    // Calcula el PageRank de cada nodo usando el grado de salida
    std::map<std::string, double> calcularPageRank(int iteraciones = 20, double d = 0.85) const {
        std::map<std::string, double> pagerank;
        std::vector<std::string> nodos;
        for (const auto& par : adj) {
            nodos.push_back(par.first);
        }
        int N = nodos.size();
        if (N == 0) return pagerank;
        double pr_inicial = 1.0 / N;
        for (const auto& nodo : nodos) {
            pagerank[nodo] = pr_inicial;
        }
        for (int it = 0; it < iteraciones; ++it) {
            std::map<std::string, double> nuevoPR;
            for (const auto& nodo : nodos) {
                nuevoPR[nodo] = (1.0 - d) / N;
            }
            for (const auto& nodo : nodos) {
                double pr = pagerank[nodo];
                int out_degree = adj.at(nodo).size();
                if (out_degree == 0) {
                    // Nodo sin salidas: distribuye su PR a todos
                    for (const auto& destino : nodos) {
                        nuevoPR[destino] += d * pr / N;
                    }
                } else {
                    for (const auto& vecino : adj.at(nodo)) {
                        nuevoPR[vecino.first] += d * pr / out_degree;
                    }
                }
            }
            pagerank = nuevoPR;
        }
        return pagerank;
    }
};
