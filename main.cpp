#include <fstream>
#include <iostream>
#include <map>
#include <sstream>
#include <string>
#include <algorithm>
#include <chrono>
#include "Document.h"
#include "LRUCache.h"
#include <fcntl.h>
#include <set>
#include <filesystem>
#include <vector>
#include "Grafo.h"
#ifdef _WIN32
#include <windows.h>
#include <io.h>
#endif

// Cargar stopwords en un arreglo (esto puede usarse para cualquier archivo de stopwords)
void cargarStopwords(const std::string& stopwordsFile, std::string sw[], int& count) {
    std::ifstream stop_words(stopwordsFile);
    if (!stop_words.is_open()) {
        std::cerr << "❌ Error al abrir el archivo de stopwords" << std::endl;
        return;
    }
    std::string line;
    count = 0;
    while (std::getline(stop_words, line) && count < 100) {
        sw[count++] = line;
    }

    stop_words.close();
}

// Verifica si una palabra es una stopword
bool esStopword(const std::string& palabra, const std::string sw[], int count) {
    for (int i = 0; i < count; i++) {
        if (palabra == sw[i]) return true;
    }
    return false;
}

// Procesa una palabra y actualiza el indice invertido
void procesarPalabra(const std::string& palabra, const std::string& url, std::map<std::string, Document*>& map, const std::string sw[], int swCount) {
    std::string word = palabra;
    std::transform(word.begin(), word.end(), word.begin(), [](unsigned char c){ return std::tolower(c); });

    if (esStopword(word, sw, swCount)) return;

    if (map.find(word) == map.end()) {
        map[word] = new Document(url);
    } else {
        Document* actual = map[word];
        while (actual != nullptr) {
            if (actual->url == url) return; // Ya existe
            actual = actual->next;
        }

        // Insertar al final de la lista enlazada
        actual = map[word];
        while (actual->next != nullptr) actual = actual->next;
        actual->next = new Document(url);
    }
}

// Procesar archivo de documentos y construye el indice invertido
void indiceInvertido(std::map<std::string, Document*>& map, const std::string& stopwordsFile, const std::string& documentosFile) {
    std::string sw[100]; //La cantidad de palabras que son stopword
    int swCount = 0; //contador
    cargarStopwords(stopwordsFile, sw, swCount); //llamamos a la funcion cargarStopwords

    std::ifstream doc(documentosFile);
    if (!doc.is_open()) {
        std::cerr << "❌ Error al abrir el archivo de documentos" << std::endl;
        return;
    }

    std::string lineas;
    while (std::getline(doc, lineas)) {
        std::string url, word;
        std::istringstream stream(lineas);

        while (stream >> word) {
            if (word.size() >= 2 && word.substr(word.size() - 2) == "||") {
                url += word;
            } else {
                procesarPalabra(word, url, map, sw, swCount);
            }
        }
    }
    doc.close();
}

// Imprimir indice invertido
void printIndice(const std::map<std::string, Document*>& indice) {
    for (const auto& par : indice) {
        std::cout << "Clave: " << par.first << std::endl;
        Document* nodo = par.second;
        while (nodo != nullptr) {
            std::cout << "    URL: " << nodo->url << std::endl;
            nodo = nodo->next;
        }
        std::cout << std::endl;
    }
}


 // Funcion auxiliar para obtener todas las URLs asociadas a una palabra
 std::set<std::string> obtenerURLs(Document* nodo) {
     std::set<std::string> urls;
     while (nodo != nullptr) {
         urls.insert(nodo->url);
         nodo = nodo->next;
     }
     return urls;
 }

// Funcion de busqueda con interseccion de documentos
std::string busqueda(std::map<std::string, Document*>& map, const std::string sw[100], int swCount, const std::string& texto) {
    std::stringstream ss(texto);
    listaString* palabras = nullptr; // Lista enlazada

    // Crear lista enlazada con las palabras ingresadas por el usuario
    std::string palabra;
    while (ss >> palabra) {
        std::transform(palabra.begin(), palabra.end(), palabra.begin(), [](unsigned char c) {
            return std::tolower(c);
        });

        if (esStopword(palabra, sw, swCount)) continue;

        listaString* nuevoNodo = new listaString(palabra);
        if (palabras == nullptr) {
            palabras = nuevoNodo;
        } else {
            listaString* aux = palabras;
            while (aux->next != nullptr) aux = aux->next;
            aux->next = nuevoNodo;
        }
    }

    // Validacion de que al menos exista una palabra valida
    if (!palabras) {
        return "⚠️ No se ingresaron palabras validas.\n";
    }

    // Sumar frecuencias de todas las URLs relacionadas con las palabras de la consulta
    std::map<std::string, int> frecuenciaURLs;
    listaString* key = palabras;
    while (key != nullptr) {
        auto it = map.find(key->value);
        if (it != map.end()) {
            Document* doc = it->second;
            while (doc != nullptr) {
                frecuenciaURLs[doc->url] += doc->frecuencia;
                doc = doc->next;
            }
        }
        key = key->next;
    }

    // Ordenar por frecuencia descendente
    std::vector<std::pair<std::string, int>> ordenado(frecuenciaURLs.begin(), frecuenciaURLs.end());
    std::sort(ordenado.begin(), ordenado.end(), [](const auto& a, const auto& b) {
        return a.second > b.second;
    });

    // Construir el string de resultado
    std::ostringstream out;
    if (ordenado.empty()) {
        out << "⚠️ No se encontraron documentos con las palabras ingresadas.\n";
    } else {
        out << "Documentos encontrados:\n";
        int count = 0;
        for (const auto& par : ordenado) {
            if (count >= 10) break; // Solo los 10 mas relevantes
            out << " - " << par.second << " - " << par.first << "\n";
            count++;
        }
    }

    // Liberar memoria de la lista enlazada
    while (palabras != nullptr) {
        listaString* temp = palabras;
        palabras = palabras->next;
        delete temp;
    }

    return out.str();
}

// Extrae los 10 documentos mas relevantes de una consulta
std::vector<std::string> obtenerDocsRelevantes(const std::map<std::string, int>& frecuenciaURLs) {
    std::vector<std::pair<std::string, int>> ordenado(frecuenciaURLs.begin(), frecuenciaURLs.end());
    std::sort(ordenado.begin(), ordenado.end(), [](const auto& a, const auto& b) {
        return a.second > b.second;
    });
    std::vector<std::string> docs;
    for (size_t i = 0; i < ordenado.size() && i < 10; ++i) {
        docs.push_back(ordenado[i].first);
    }
    return docs;
}

// Construye el grafo a partir de los resultados de cada consulta (solo no dirigido)
void construirGrafo(Grafo& grafo, const std::vector<std::string>& docsRelevantes) {
    for (size_t i = 0; i < docsRelevantes.size(); ++i) {
        for (size_t j = i + 1; j < docsRelevantes.size(); ++j) {
            grafo.agregarAristaNoDirigida(docsRelevantes[i], docsRelevantes[j]);
        }
    }
}

// Interfaz interactiva para consultas
void interfazConsultas(const Grafo& grafo, const std::map<std::string, double>& pagerank, LRUCache* cache, std::map<std::string, Document*>& map, const std::string sw[], int swCount, int& totalConsultas, int& hits, int& misses, int& inserciones, int& reemplazos) {
    std::cout << "\n=== INTERFAZ DE CONSULTAS INTERACTIVA ===\n";
    std::cout << "Comandos disponibles:\n";
    std::cout << "1. 'grafo <url>'          - Mostrar conexiones de un nodo en el grafo\n";
    std::cout << "2. 'pagerank <url>'       - Mostrar PageRank de un documento\n";
    std::cout << "3. 'cache <consulta>'     - Verificar si una consulta esta en cache\n";
    std::cout << "4. 'buscar <consulta>'    - Realizar busqueda y cachear resultado\n";
    std::cout << "5. 'top-pagerank <n>'     - Mostrar top N documentos por PageRank\n";
    std::cout << "6. 'mostrar-cache'        - Mostrar todo el contenido del cache\n";
    std::cout << "7. 'limpiar-cache'        - Eliminar todo el contenido del cache y reiniciar estadísticas\n";
    std::cout << "8. 'stats'                - Mostrar estadisticas generales\n";
    std::cout << "9. 'salir'                - Terminar interfaz\n";
    std::cout << "=========================================\n";
    
    std::string comando;
    while (true) {
        std::cout << "\n> Ingrese comando: ";
        std::getline(std::cin, comando);
        
        if (comando == "salir" || comando == "exit") {
            std::cout << "Saliendo de la interfaz...\n";
            break;
        }
        
        // Parsear comando
        std::istringstream iss(comando);
        std::string accion, parametro;
        iss >> accion;
        std::getline(iss, parametro);
        
        // Eliminar espacios al inicio del parametro
        if (!parametro.empty() && parametro[0] == ' ') {
            parametro = parametro.substr(1);
        }
        
        if (accion == "grafo") {
            if (parametro.empty()) {
                std::cout << "⚠️ Especifique una URL. Ejemplo: grafo <url>\n";
                continue;
            }
            
            // Buscar en el grafo
            auto it = grafo.adj.find(parametro);
            if (it != grafo.adj.end()) {
                std::cout << "Documento '" << parametro << "' encontrado en el grafo\n";
                std::cout << "Conexiones (" << it->second.size() << " documentos relacionados):\n";
                int count = 0;
                for (const auto& vecino : it->second) {
                    std::cout << "  " << (++count) << ". " << vecino.first << " (peso: " << vecino.second << ")\n";
                    if (count >= 10) {
                        std::cout << "  ... y " << (it->second.size() - 10) << " mas\n";
                        break;
                    }
                }
            } else {
                std::cout << "Documento '" << parametro << "' no encontrado en el grafo\n";
            }
        }
        else if (accion == "pagerank") {
            if (parametro.empty()) {
                std::cout << "⚠️ Especifique una URL. Ejemplo: pagerank <url>\n";
                continue;
            }
            
            auto it = pagerank.find(parametro);
            if (it != pagerank.end()) {
                std::cout << "PageRank de '" << parametro << "': " << it->second << "\n";
                
                // Calcular ranking relativo
                int mejoresQue = 0;
                for (const auto& pr : pagerank) {
                    if (pr.second > it->second) mejoresQue++;
                }
                std::cout << "Ranking: #" << (mejoresQue + 1) << " de " << pagerank.size() << " documentos\n";
            } else {
                std::cout << "Documento '" << parametro << "' no tiene PageRank calculado\n";
            }
        }
        else if (accion == "cache") {
            if (parametro.empty()) {
                std::cout << "⚠️ Especifique una consulta. Ejemplo: cache <consulta>\n";
                continue;
            }
            
            if (cache->contains(parametro)) {
                std::cout << "Consulta '" << parametro << "' esta en cache\n";
                std::string resultado = cache->get(parametro);
                std::cout << "Resultado cacheado:\n";
                // Mostrar solo las primeras 3 lineas del resultado
                std::istringstream stream(resultado);
                std::string linea;
                int lineas = 0;
                while (std::getline(stream, linea) && lineas < 3) {
                    std::cout << "  " << linea << "\n";
                    lineas++;
                }
                if (lineas >= 3) std::cout << "  ...\n";
            } else {
                std::cout << "Consulta '" << parametro << "' no esta en cache\n";
            }
        }
        else if (accion == "buscar") {
            if (parametro.empty()) {
                std::cout << "⚠️ Especifique una consulta. Ejemplo: buscar government policy\n";
                continue;
            }
            
            std::cout << "Buscando: '" << parametro << "'\n";
            totalConsultas++; // Incrementar total de consultas procesadas
            
            // Verificar si ya esta en cache
            if (cache->contains(parametro)) {
                std::string resultadoCache = cache->get(parametro);
                std::cout << "[CACHE HIT] Resultado encontrado en cache:\n";
                std::cout << resultadoCache;
                hits++; // Incrementar hits
            } else {
                std::cout << "[CACHE MISS] Calculando resultado...\n";
                // Realizar busqueda
                std::string resultado = busqueda(map, sw, swCount, parametro);
                
                // Solo cachear si el resultado es valido (no es "No se encontraron documentos...")
                if (resultado.find("No se encontraron documentos") == std::string::npos && 
                    resultado.find("No se ingresaron palabras") == std::string::npos) {
                    
                    // Verificar si el cache esta lleno para contar reemplazos
                    if (cache->getSize() >= cache->getMaxSize()) {
                        reemplazos++; // Se va a eliminar un elemento
                    }
                    
                    // Cachear el resultado valido
                    cache->put(parametro, resultado);
                    inserciones++; // Incrementar inserciones
                    std::cout << "Resultado calculado y guardado en cache:\n";
                } else {
                    std::cout << "Resultado calculado (no cacheado - sin resultados):\n";
                }
                
                misses++; // Incrementar misses
                std::cout << resultado;
            }
        }
        else if (accion == "top-pagerank") {
            int n = 10; // Por defecto
            if (!parametro.empty()) {
                try {
                    n = std::stoi(parametro);
                } catch (const std::exception&) {
                    std::cout << "⚠️ Numero invalido. Usando 10 por defecto.\n";
                    n = 10;
                }
            }
            
            // Ordenar por PageRank
            std::vector<std::pair<std::string, double>> rankingPR;
            for (const auto& pr : pagerank) {
                rankingPR.push_back({pr.first, pr.second});
            }
            std::sort(rankingPR.begin(), rankingPR.end(), [](const auto& a, const auto& b) {
                return a.second > b.second;
            });
            
            std::cout << "Top " << n << " documentos por PageRank:\n";
            for (int i = 0; i < std::min(n, (int)rankingPR.size()); i++) {
                std::cout << "  " << (i+1) << ". " << rankingPR[i].second 
                         << " - " << rankingPR[i].first << "\n";
            }
        }
        else if (accion == "mostrar-cache") {
            std::cout << "📋 Contenido completo del cache LRU:\n";
            std::cout << "   Estado actual: " << cache->getSize() << "/" << cache->getMaxSize() << " elementos\n\n";
            
            if (cache->getSize() == 0) {
                std::cout << "   ⚠️ El cache esta vacio\n";
            } else {
                // Obtener todas las consultas del cache
                std::vector<std::string> consultasEnCache = cache->getAllKeys();
                
                std::cout << "   === CONSULTAS EN CACHE (ordenadas por uso reciente) ===\n";
                for (size_t i = 0; i < consultasEnCache.size(); i++) {
                    std::cout << "   [" << (i+1) << "] Consulta: \"" << consultasEnCache[i] << "\"\n";
                    
                    // Obtener el resultado (esto también actualiza el LRU)
                    std::string resultado = cache->get(consultasEnCache[i]);
                    
                    // Mostrar las primeras 2 líneas del resultado
                    std::istringstream stream(resultado);
                    std::string linea;
                    int lineasMostradas = 0;
                    std::cout << "       Resultado:\n";
                    while (std::getline(stream, linea) && lineasMostradas < 2) {
                        std::cout << "         " << linea << "\n";
                        lineasMostradas++;
                    }
                    if (lineasMostradas >= 2) {
                        std::cout << "         ...\n";
                    }
                    std::cout << "\n";
                }
            }
        }
        else if (accion == "stats") {
            std::cout << "📊 Estadisticas del sistema:\n";
            std::cout << "  -> Nodos en el grafo: " << grafo.adj.size() << "\n";
            
            int totalAristas = 0;
            for (const auto& par : grafo.adj) {
                totalAristas += par.second.size();
            }
            totalAristas /= 2; // No dirigido
            std::cout << "  -> Aristas en el grafo: " << totalAristas << "\n";
            std::cout << "  -> Documentos con PageRank: " << pagerank.size() << "\n";
            
            // Metricas detalladas del cache
            std::cout << "\n  === METRICAS DEL CACHE LRU ===\n";
            std::cout << "  -> Total de consultas procesadas: " << totalConsultas << "\n";
            std::cout << "  -> Total de aciertos [Hits]: " << hits << "\n";
            std::cout << "  -> Total de fallos [Misses]: " << misses << "\n";
            
            if (totalConsultas > 0) {
                double tasaAciertos = (hits * 100.0) / totalConsultas;
                double tasaFallos = (misses * 100.0) / totalConsultas;
                std::cout << "  -> Tasa de aciertos: " << tasaAciertos << "%\n";
                std::cout << "  -> Tasa de fallos: " << tasaFallos << "%\n";
            } else {
                std::cout << "  -> Tasa de aciertos: 0%\n";
                std::cout << "  -> Tasa de fallos: 0%\n";
            }
            
            std::cout << "  -> Numero de reemplazos/eliminaciones: " << reemplazos << "\n";
            std::cout << "  -> Numero de inserciones en cache: " << inserciones << "\n";
            std::cout << "  -> Elementos actuales en cache: " << cache->getSize() << "/" << cache->getMaxSize() << "\n";
            
            if (!pagerank.empty()) {
                auto maxPR = std::max_element(pagerank.begin(), pagerank.end(),
                    [](const auto& a, const auto& b) { return a.second < b.second; });
                auto minPR = std::min_element(pagerank.begin(), pagerank.end(),
                    [](const auto& a, const auto& b) { return a.second < b.second; });
                std::cout << "\n  -> PageRank maximo: " << maxPR->second << " (" << maxPR->first << ")\n";
                std::cout << "  -> PageRank minimo: " << minPR->second << " (" << minPR->first << ")\n";
            }
        }
        else if (accion == "limpiar-cache") {
            std::cout << "🗑️ Limpiando todo el contenido del cache y estadísticas...\n";
            
            int elementosEliminados = cache->getSize();
            cache->clear();
            
            // Reiniciar estadísticas del cache
            hits = 0;
            misses = 0;
            totalConsultas = 0;
            inserciones = 0;
            reemplazos = 0;
            
            std::cout << "✅ Cache y estadísticas limpiados exitosamente\n";
            std::cout << "   Elementos eliminados: " << elementosEliminados << "\n";
            std::cout << "   Estado actual: " << cache->getSize() << "/" << cache->getMaxSize() << " elementos\n";
            std::cout << "   Estadísticas reiniciadas: Hits=0, Misses=0, Total=0\n";
        }
        else {
            std::cout << "⚠️ Comando no reconocido. Use 'salir' para terminar.\n";
            std::cout << "📝 Comandos: grafo, pagerank, cache, buscar, top-pagerank, mostrar-cache, limpiar-cache, stats, salir\n";
        }
    }
}

// Funcion principal
int main() {
    // Configurar consola para UTF-8 en Windows (Es por que le pusimos emojis al codigo)
    #ifdef _WIN32
        SetConsoleOutputCP(CP_UTF8);
        SetConsoleCP(CP_UTF8);
        // Habilitar secuencias ANSI para emojis
        HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
        DWORD dwMode = 0;
        GetConsoleMode(hOut, &dwMode);
        dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
        SetConsoleMode(hOut, dwMode);
    #endif
    
    std::filesystem::path exePath = std::filesystem::current_path();

    std::string file_I = (exePath / "gov1_pages.dat").string();
    std::string file_C = (exePath / "gov2_pages.dat").string();
    std::string stopwords = (exePath / "stopwords_english.dat.txt").string();
    
    std::string logQueries = (exePath / "Log-Queries.dat").string();
    std::string resultadosFile = (exePath / "ResultQueries.txt").string();

    std::map<std::string , Document*> map;
    
    // Crear cache LRU con tamano fijo de 50 consultas en el cache y metricas
    LRUCache* cache = new LRUCache(50);
    int cacheHits = 0;
    int cacheMisses = 0;
    int totalConsultasProcesadas = 0;
    int cacheInserciones = 0;
    int cacheReemplazos = 0;

    std::cout << "Cargando stopwords...\n";
    std::string sw[100];
    int swCount = 0;
    cargarStopwords(stopwords, sw, swCount);
    std::cout << "✅ Stopwords cargadas\n\n";

    std::cout << "Construyendo indice invertido...\n";
    indiceInvertido(map, stopwords, file_I);
    std::cout << "✅ Indice invertido construido\n\n";

    std::cout << "Abriendo archivos de queries y resultados...\n";
    std::ifstream queries(logQueries);
    std::ofstream resultados(resultadosFile);
    if (!queries.is_open()) {
        std::cerr << "No se pudo abrir Log-Queries.dat\n";
        return 1;
    }
    if (!resultados.is_open()) {
        std::cerr << "No se pudo crear ResultQueries.txt\n";
        return 1;
    }
    std::cout << "✅ Se abrieron correctamente los archivos\n\n";

    std::string consulta;
    int numConsulta = 1;
    Grafo grafo;
    int totalAristas = 0;
    std::vector<std::string> consultasEjemplo;
    std::vector<std::vector<std::string>> docsPorConsulta;
    std::vector<std::string> resultadosSinPR;

    std::cout << "Procesando queries y generando ResultQueries.txt y grafo de co-relevancia...\n";
    auto start_grafo = std::chrono::high_resolution_clock::now();

    while (std::getline(queries, consulta)) {
        std::string resultado;
        totalConsultasProcesadas++; // Contar consulta procesada
        
        // Verificar si la consulta esta en cache
        if (cache->contains(consulta)) {
            // Cache hit
            resultado = cache->get(consulta);
            cacheHits++;
        } else {
            // Cache miss - calcular resultado
            resultado = busqueda(map, sw, swCount, consulta);
            
            // Solo cachear si el resultado es valido (no es "No se encontraron documentos...")
            if (resultado.find("No se encontraron documentos") == std::string::npos && 
                resultado.find("No se ingresaron palabras") == std::string::npos) {
                
                // Verificar si el cache esta lleno para contar reemplazos
                if (cache->getSize() >= cache->getMaxSize()) {
                    cacheReemplazos++;
                }
                
                cache->put(consulta, resultado);
                cacheInserciones++; // Contar insercion
            }
            
            cacheMisses++;
        }

        // --- Nuevo: obtener documentos relevantes y construir el grafo ---
        std::map<std::string, int> frecuenciaURLs;
        listaString* palabras = nullptr;
        std::istringstream ss(consulta);
        std::string palabra;
        while (ss >> palabra) {
            std::transform(palabra.begin(), palabra.end(), palabra.begin(), [](unsigned char c) {
                return std::tolower(c);
            });
            if (esStopword(palabra, sw, swCount)) continue;
            listaString* nuevoNodo = new listaString(palabra);
            if (palabras == nullptr) {
                palabras = nuevoNodo;
            } else {
                listaString* aux = palabras;
                while (aux->next != nullptr) aux = aux->next;
                aux->next = nuevoNodo;
            }
        }
        listaString* key = palabras;
        while (key != nullptr) {
            auto it = map.find(key->value);
            if (it != map.end()) {
                Document* doc = it->second;
                while (doc != nullptr) {
                    frecuenciaURLs[doc->url] += doc->frecuencia;
                    doc = doc->next;
                }
            }
            key = key->next;
        }
        std::vector<std::string> docsRelevantes = obtenerDocsRelevantes(frecuenciaURLs);
        totalAristas += (docsRelevantes.size() * (docsRelevantes.size() - 1)) / 2;
        construirGrafo(grafo, docsRelevantes);
        resultados << "Consulta #" << numConsulta << ": " << consulta << "\n";
        resultados << resultado << "\n";
        // Guardar para comparacion de ranking
        if (consultasEjemplo.size() < 5) {
            consultasEjemplo.push_back(consulta);
            docsPorConsulta.push_back(docsRelevantes);
            resultadosSinPR.push_back(resultado);
        }
        numConsulta++;
    }
    std::cout << "✅ Consultas procesadas y resultados guardados en ResultQueries.txt\n\n";
    queries.close();
    resultados.close();
    auto end_grafo = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double, std::milli> tiempoConstruccionGrafo = end_grafo - start_grafo;

    // Metricas del grafo
    int numNodos = grafo.adj.size();
    int numAristas = 0;
    for (const auto& par : grafo.adj) {
        numAristas += par.second.size();
    }
    numAristas /= 2; // No dirigido

    std::cout << "  -> Numero total de consultas usadas para construir el grafo: " << numConsulta-1 << std::endl;
    std::cout << "  -> Numero de nodos en el grafo: " << numNodos << std::endl;
    std::cout << "  -> Numero de aristas en el grafo: " << numAristas << std::endl;
    std::cout << "  -> Tiempo de construccion del grafo: " << tiempoConstruccionGrafo.count() << " ms\n\n";

    std::cout << "Guardando la lista de adyacencia en ListaAdyacencia.txt...\n";
    grafo.guardarEnArchivo("ListaAdyacencia.txt");
    std::cout << "✅ Grafo guardado en ListaAdyacencia.txt\n\n";

    // Calcular PageRank y medir tiempo
    std::cout << "Calculando PageRank...\n";
    auto start_pr = std::chrono::high_resolution_clock::now();
    int iteracionesPR = 0;
    std::map<std::string, double> pagerank;
    double d = 0.85;
    int maxIter = 100;

    // PageRank con convergencia
    pagerank = grafo.calcularPageRank(maxIter, d);
    iteracionesPR = maxIter; // (No hay convergencia real, solo iteraciones fijas)
    auto end_pr = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double, std::milli> tiempoPR = end_pr - start_pr;
    std::cout << "✅ PageRank calculado\n\n";
    std::cout << "  -> Iteraciones de PageRank: " << iteracionesPR << std::endl;
    std::cout << "  -> Tiempo de calculo de PageRank: " << tiempoPR.count() << " ms\n";

    // Interfaz interactiva para consultas
    std::cout << "\nDesea usar la interfaz de consultas interactiva? (s/n): ";
    char respuesta;
    std::cin >> respuesta;
    std::cin.ignore(); // Limpiar buffer
    
    if (respuesta == 's' || respuesta == 'S') {
        interfazConsultas(grafo, pagerank, cache, map, sw, swCount, totalConsultasProcesadas, cacheHits, cacheMisses, cacheInserciones, cacheReemplazos);
    }

    std::cout << "\nPrograma finalizado correctamente.\n";
    
    // Liberar memoria del cache
    delete cache;
    return 0;
}
