#pragma once

#include <JuceHeader.h>

// Inclusion complète des headers Qt requis
#include <QtCore/QEventLoop>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include <QtCore/QJsonArray>
#include <QtCore/QFile>
#include <QtCore/QUrl>
#include <QtCore/QTimer>
#include <QtCore/QProcess>
#include <QtCore/QFileInfo>
#include <QtCore/QDir>
#include <QtCore/QStringList>
#include <QtNetwork/QNetworkAccessManager>
#include <QtNetwork/QNetworkReply>
#include <QtNetwork/QNetworkRequest>

// Inclure une copie fonctionnelle de votre client Qt originel (en le renommant pour éviter les conflits)
// Cette classe est une copie exacte de tangoflux_client.h avec un préfixe QtImpl_
#include <random>
#include <string>
#include <fstream>
#include <chrono>
#include <thread>
#include <iostream>

// Classe TangoFluxClient Qt d'origine, renommée pour éviter les conflits
class QtImpl_TangoFluxClient {
private:
    std::string server_url;
    bool verbose;
    std::string session_hash;

    // Génère un identifiant de session aléatoire
    std::string generate_session_hash() {
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> dis(0, 35);
        const char* chars = "abcdefghijklmnopqrstuvwxyz0123456789";
        std::string result;
        result.reserve(12);
        for (int i = 0; i < 12; i++) {
            result += chars[dis(gen)];
        }
        return result;
    }

    // Effectue une requête POST
    std::string make_post_request(const std::string& url, const std::string& data) {
        if (verbose) {
            std::cout << "POST " << url << std::endl;
            std::cout << "DATA: " << data << std::endl;
        }

        // Créer un QNetworkAccessManager local pour éviter les problèmes de thread
        QNetworkAccessManager localManager;
        QNetworkRequest request(QUrl(QString::fromStdString(url)));
        request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

        QEventLoop loop;
        QNetworkReply* reply = localManager.post(request, QByteArray(data.c_str()));

        QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
        loop.exec();

        if (reply->error() != QNetworkReply::NoError) {
            std::string error = "Erreur de réseau: " + reply->errorString().toStdString();
            reply->deleteLater();
            throw std::runtime_error(error);
        }

        QByteArray responseData = reply->readAll();
        reply->deleteLater();

        if (verbose) {
            std::cout << "Réponse: " << QString(responseData).toStdString() << std::endl;
        }

        return QString(responseData).toStdString();
    }

    // Extrait une valeur d'une chaîne JSON
    std::string extract_json_value(const std::string& json_str, const std::string& key) {
        QJsonDocument doc = QJsonDocument::fromJson(QByteArray(json_str.c_str()));
        if (doc.isNull() || !doc.isObject()) {
            return "";
        }

        QJsonObject json = doc.object();
        if (json.contains(QString::fromStdString(key))) {
            QJsonValue value = json[QString::fromStdString(key)];
            if (value.isString()) {
                return value.toString().toStdString();
            }
        }
        return "";
    }

    // Fonction pour écouter les événements SSE et récupérer le chemin du fichier
    bool listen_for_sse_events(const std::string& session_hash_param, std::string& file_path) {
        std::string sse_url = server_url + "/gradio_api/queue/data?session_hash=" + session_hash_param;

        if (verbose) {
            std::cout << "Connexion au flux d'événements SSE: " << sse_url << std::endl;
            std::cout << "Écoute des événements SSE..." << std::endl;
        }

        try {
            // Créer un QNetworkAccessManager local pour éviter les problèmes de thread
            QNetworkAccessManager localManager;
            QNetworkRequest request(QUrl(QString::fromStdString(sse_url)));
            request.setRawHeader("Accept", "text/event-stream");
            request.setRawHeader("Cache-Control", "no-cache");

            QEventLoop loop;
            QNetworkReply* reply = localManager.get(request);

            // Définir un timeout de 10 secondes
            QTimer::singleShot(10000, &loop, &QEventLoop::quit);

            QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
            loop.exec();

            if (reply->error() == QNetworkReply::NoError) {
                QByteArray responseData = reply->readAll();
                std::string response = QString(responseData).toStdString();

                if (verbose) {
                    std::cout << "\n===== RÉPONSE SSE =====" << std::endl;
                    std::cout << response << std::endl;
                    std::cout << "======================" << std::endl;
                }

                // Rechercher un chemin de fichier WAV dans la réponse
                size_t wav_pos = response.find(".wav");
                if (wav_pos != std::string::npos) {
                    // Chercher un chemin complet avec /tmp/
                    size_t path_start = response.rfind("/tmp/", wav_pos);
                    if (path_start != std::string::npos) {
                        // Reculer jusqu'au premier guillemet ou début de délimiteur
                        size_t quote_start = response.rfind("\"", path_start);
                        if (quote_start != std::string::npos && quote_start < path_start) {
                            path_start = quote_start + 1;
                            size_t path_end = response.find("\"", wav_pos);
                            if (path_end != std::string::npos) {
                                std::string full_path = response.substr(path_start, path_end - path_start);
                                file_path = "/gradio_api/file=" + full_path;
                                if (verbose) {
                                    std::cout << "Chemin complet extrait: " << file_path << std::endl;
                                }
                                reply->deleteLater();
                                return true;
                            }
                        }
                    }
                }
            }
            reply->deleteLater();

            return false;
        }
        catch (const std::exception& e) {
            std::cerr << "Exception lors de l'écoute SSE: " << e.what() << std::endl;
            return false;
        }
    }

    // Attend le résultat de la génération
    std::string wait_for_result(const std::string& event_id) {
        if (verbose) {
            std::cout << "Attente du résultat (event_id: " << event_id << ")..." << std::endl;
        }

        std::string file_path;
        int attempts = 0;
        const int max_attempts = 30;

        // Stratégie 1: Écouter les événements SSE
        if (verbose) {
            std::cout << "Stratégie 1: Écoute des événements SSE..." << std::endl;
        }

        if (listen_for_sse_events(session_hash, file_path)) {
            return file_path;
        }

        // Stratégie 2: Polling avec API de statut
        if (verbose) {
            std::cout << "Stratégie 2: Vérification du statut..." << std::endl;
        }

        std::string status_url = server_url + "/gradio_api/queue/status?event_id=" + event_id;

        while (file_path.empty() && attempts < max_attempts) {
            // Créer un QNetworkAccessManager local pour éviter les problèmes de thread
            QNetworkAccessManager localManager;
            QNetworkRequest request(QUrl(QString::fromStdString(status_url)));
            QEventLoop loop;
            QNetworkReply* reply = localManager.get(request);

            QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
            loop.exec();

            if (reply->error() == QNetworkReply::NoError) {
                QByteArray responseData = reply->readAll();
                std::string response = QString(responseData).toStdString();

                // Chercher "process_completed" dans la réponse
                if (response.find("process_completed") != std::string::npos) {
                    // Rechercher le chemin du fichier dans la réponse
                    size_t wav_pos = response.find(".wav");
                    if (wav_pos != std::string::npos) {
                        // Chercher un chemin complet avec /tmp/
                        size_t path_start = response.rfind("/tmp/", wav_pos);
                        if (path_start != std::string::npos) {
                            // Reculer jusqu'au premier guillemet ou début de délimiteur
                            size_t quote_start = response.rfind("\"", path_start);
                            if (quote_start != std::string::npos && quote_start < path_start) {
                                path_start = quote_start + 1;
                                size_t path_end = response.find("\"", wav_pos);
                                if (path_end != std::string::npos) {
                                    std::string full_path = response.substr(path_start, path_end - path_start);
                                    file_path = "/gradio_api/file=" + full_path;
                                    if (verbose) {
                                        std::cout << "Chemin complet extrait: " << file_path << std::endl;
                                    }
                                    break;
                                }
                            }
                        }
                    }
                }
            }

            reply->deleteLater();

            // Attendre avant la prochaine tentative
            std::this_thread::sleep_for(std::chrono::seconds(1));
            attempts++;

            if (verbose && attempts % 5 == 0) {
                std::cout << "Tentative " << attempts << "/" << max_attempts << "..." << std::endl;
            }
        }

        if (file_path.empty()) {
            throw std::runtime_error("Timeout: Aucun résultat reçu après " + std::to_string(max_attempts) + " secondes");
        }

        if (verbose) {
            std::cout << "Résultat reçu: " << file_path << std::endl;
        }

        return file_path;
    }

    // Télécharge un fichier à partir d'une URL
    bool download_file(const std::string& url, const std::string& output_file) {
        // Créer un QNetworkAccessManager local pour éviter les problèmes de thread
        QNetworkAccessManager localManager;
        QNetworkRequest request((QUrl(QString::fromStdString(url))));

        QEventLoop loop;
        QNetworkReply* reply = localManager.get(request);

        QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
        loop.exec();

        if (reply->error() != QNetworkReply::NoError) {
            std::cerr << "Erreur de téléchargement: " << reply->errorString().toStdString() << std::endl;
            reply->deleteLater();
            return false;
        }

        QFile file(QString::fromStdString(output_file));
        if (!file.open(QIODevice::WriteOnly)) {
            std::cerr << "Erreur: Impossible d'ouvrir le fichier de sortie: " << output_file << std::endl;
            reply->deleteLater();
            return false;
        }

        file.write(reply->readAll());
        file.close();
        reply->deleteLater();

        return true;
    }

public:
    // Constructeur
    QtImpl_TangoFluxClient(const std::string& url, bool verbose_mode = false)
        : server_url(url), verbose(verbose_mode) {
        // Nettoyer l'URL si nécessaire
        if (!server_url.empty() && server_url.back() == '/') {
            server_url.pop_back();
        }

        // Générer un identifiant de session unique
        session_hash = generate_session_hash();

        if (verbose) {
            std::cout << "Client initialisé" << std::endl;
            std::cout << "URL du serveur: " << server_url << std::endl;
            std::cout << "Session hash: " << session_hash << std::endl;
        }
    }

    // Destructeur
    ~QtImpl_TangoFluxClient() {
        // Pas besoin de nettoyage explicite pour Qt
    }

    // Génère un audio avec TangoFlux via l'API Gradio
    std::string generate_audio(
        const std::string& prompt,
        float duration = 5.0f,
        int steps = 50,
        float guidance_scale = 3.5f,
        int seed = 42,
        const std::string& output_file = "output.wav"
    ) {
        if (verbose) {
            std::cout << "Génération d'audio avec les paramètres:" << std::endl;
            std::cout << "- Prompt: " << prompt << std::endl;
            std::cout << "- Durée: " << duration << "s" << std::endl;
            std::cout << "- Étapes: " << steps << std::endl;
            std::cout << "- Guidance Scale: " << guidance_scale << std::endl;
            std::cout << "- Seed: " << seed << std::endl;
        }

        // 1. Rejoindre la file d'attente pour démarrer la génération
        std::string join_url = server_url + "/gradio_api/queue/join";

        std::string join_data = "{"
            "\"data\": [\"" + prompt + "\", " +
            std::to_string(duration) + ", " +
            std::to_string(steps) + ", " +
            std::to_string(guidance_scale) + ", " +
            std::to_string(seed) + "]," +
            "\"event_data\": null," +
            "\"fn_index\": 0," +
            "\"trigger_id\": 0," +
            "\"session_hash\": \"" + session_hash + "\""
            "}";

        if (verbose) {
            std::cout << "Rejoindre la file d'attente Gradio..." << std::endl;
        }

        std::string join_response = make_post_request(join_url, join_data);
        std::string event_id = extract_json_value(join_response, "event_id");

        if (event_id.empty()) {
            throw std::runtime_error("Impossible d'obtenir l'identifiant d'événement de la file d'attente");
        }

        if (verbose) {
            std::cout << "Demande de génération envoyée avec succès (event_id: " << event_id << ")" << std::endl;
        }

        // 2. Attendre le résultat
        std::string file_path = wait_for_result(event_id);

        if (file_path.empty()) {
            throw std::runtime_error("Aucun fichier audio n'a été généré");
        }

        // 3. Télécharger le fichier audio
        std::string file_url;
        if (file_path.find("http") == 0) {
            file_url = file_path;
        }
        else {
            if (file_path[0] != '/') {
                file_url = server_url + "/" + file_path;
            }
            else {
                file_url = server_url + file_path;
            }
        }

        if (verbose) {
            std::cout << "Téléchargement du fichier audio: " << file_url << std::endl;
        }

        if (!download_file(file_url, output_file)) {
            throw std::runtime_error("Échec du téléchargement du fichier audio");
        }

        if (verbose) {
            std::cout << "Audio généré avec succès et sauvegardé dans: " << output_file << std::endl;
        }

        return output_file;
    }

    // Accesseurs
    void set_server_url(const std::string& url) {
        server_url = url;
        if (!server_url.empty() && server_url.back() == '/') {
            server_url.pop_back();
        }
    }

    std::string get_server_url() const {
        return server_url;
    }

    void set_verbose(bool v) {
        verbose = v;
    }

    bool is_verbose() const {
        return verbose;
    }
};

// Classe adaptateur pour intégrer le client Qt dans JUCE
class TangoFluxClient : public juce::Thread
{
public:
    // Structure pour les paramètres de génération
    struct GenerationParams {
        juce::String prompt;
        float duration = 5.0f;
        int steps = 50;
        float guidanceScale = 3.5f;
        int seed = 42;
    };

    // Événements
    class Listener
    {
    public:
        virtual ~Listener() = default;
        virtual void generationCompleted(const juce::File& irFile) = 0;
        virtual void generationFailed(const juce::String& errorMessage) = 0;
        virtual void generationProgress(float progressPercentage) = 0;
    };

    // Constructeur et destructeur
    TangoFluxClient()
        : juce::Thread("TangoFluxClient"),
        qtClient("https://86d451fde387122f93.gradio.live", false),
        verbose(false)
    {
        // Initialisation minimale
        serverUrl = juce::String(qtClient.get_server_url());
    }

    ~TangoFluxClient() override
    {
        // Arrêter le thread s'il est en cours d'exécution
        stopThread(5000);
    }

    // Méthodes principales
    void generateIR(const GenerationParams& params, const juce::File& outFile)
    {
        // Ne pas démarrer si déjà en cours d'exécution
        if (isThreadRunning())
            return;

        // Stocker les paramètres pour le thread
        currentParams = params;
        outputFile = outFile;

        // Mettre à jour le statut
        statusMessage = "Starting generation...";

        // Démarrer le thread
        startThread();
    }

    void setServerUrl(const juce::String& url)
    {
        serverUrl = url;
        if (serverUrl.endsWith("/"))
            serverUrl = serverUrl.dropLastCharacters(1);

        qtClient.set_server_url(serverUrl.toStdString());
    }

    juce::String getServerUrl() const
    {
        return serverUrl;
    }

    juce::String getStatusMessage() const
    {
        return statusMessage;
    }

    void setVerboseMode(bool verboseMode)
    {
        verbose = verboseMode;
        qtClient.set_verbose(verbose);
    }

    // Gestion des écouteurs
    void addListener(Listener* listener)
    {
        juce::ScopedLock lock(listenerLock);
        listeners.add(listener);
    }

    void removeListener(Listener* listener)
    {
        juce::ScopedLock lock(listenerLock);
        listeners.remove(listener);
    }

private:
    // Méthode exécutée dans le thread
    void run() override
    {
        try
        {
            // Notification de progression 10%
            {
                juce::ScopedLock lock(listenerLock);
                listeners.call(&Listener::generationProgress, 0.1f);
            }

            statusMessage = "Generating... Please wait";

            // Utiliser notre client Qt pour générer l'audio
            qtClient.generate_audio(
                currentParams.prompt.toStdString(),
                currentParams.duration,
                currentParams.steps,
                currentParams.guidanceScale,
                currentParams.seed,
                outputFile.getFullPathName().toStdString()
            );

            statusMessage = "IR generated successfully";

            // Notification de fin de génération
            {
                juce::ScopedLock lock(listenerLock);
                listeners.call(&Listener::generationCompleted, outputFile);
                listeners.call(&Listener::generationProgress, 1.0f);
            }
        }
        catch (const std::exception& e)
        {
            statusMessage = "Error: " + juce::String(e.what());

            // Notification d'erreur
            {
                juce::ScopedLock lock(listenerLock);
                listeners.call(&Listener::generationFailed, statusMessage);
            }
        }
    }

    // Variables membres
    QtImpl_TangoFluxClient qtClient;  // Instance du client Qt fonctionnel
    juce::String serverUrl;
    bool verbose;

    GenerationParams currentParams;
    juce::File outputFile;
    juce::String statusMessage;
    juce::String sessionHash;

    juce::ListenerList<Listener> listeners;
    juce::CriticalSection listenerLock;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TangoFluxClient)
};