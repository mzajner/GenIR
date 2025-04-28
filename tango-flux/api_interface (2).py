import gradio as gr
import torchaudio
import tempfile
import torch
import os
import json
import time
from safetensors.torch import load_file
from huggingface_hub import snapshot_download
from diffusers import AutoencoderOobleck
from tangoflux.model import TangoFlux

# Chemins vers le modèle et les ressources
MODEL_PATH = "/teamspace/studios/this_studio/.lightning_studio/Tangoflux/TangoFlux/TangoFlux/outputs/epoch_65"
resources_path = "/teamspace/studios/this_studio/.lightning_studio/Tangoflux/resources"
vae_path = "/teamspace/studios/this_studio/.lightning_studio/Tangoflux/resources/vae.safetensors"
config_path = "/teamspace/studios/this_studio/.lightning_studio/Tangoflux/TangoFlux/TangoFlux/configs/tangoflux_config.yaml"

# Vérifier que le modèle existe
if not os.path.exists(MODEL_PATH):
    raise FileNotFoundError(f"Le chemin du modèle {MODEL_PATH} n'existe pas.")

# Chercher spécifiquement model_1.safetensors
model_file = os.path.join(MODEL_PATH, "model_1.safetensors")
if not os.path.exists(model_file):
    raise FileNotFoundError(f"Le fichier model_1.safetensors n'existe pas dans {MODEL_PATH}")

# Configuration du dispositif
device = "cuda" if torch.cuda.is_available() else "cpu"
print(f"Utilisation du dispositif: {device}")

# Initialiser le VAE
print("Initialisation du VAE...")
vae = AutoencoderOobleck()
vae_weights = load_file(vae_path)
vae.load_state_dict(vae_weights)
vae.to(device)
vae.eval()

# Charger la configuration
print("Chargement de la configuration...")
try:
    # Si c'est un fichier YAML
    import yaml
    with open(config_path, "r") as f:
        config = yaml.safe_load(f)
    
    # Si c'est un fichier YAML avec une section 'model', extraire cette section
    if "model" in config:
        config = config["model"]
except Exception as e:
    print(f"Erreur lors du chargement du YAML: {e}")
    # Essayons de charger comme JSON
    try:
        with open(config_path, "r") as f:
            config = json.load(f)
    except Exception as e2:
        print(f"Erreur lors du chargement du JSON: {e2}")

# Initialiser le modèle
print("Initialisation du modèle TangoFlux...")
model = TangoFlux(config)
model_weights = load_file(model_file)
model.load_state_dict(model_weights, strict=False)
model.to(device)
model.eval()
print("Modèle chargé avec succès!")

def generate_audio(prompt, duration, steps, guidance_scale=3.5, seed=42):
    """Fonction de génération pour l'interface Gradio et l'API"""
    start_time = time.time()
    print(f"Génération avec prompt: '{prompt}', durée: {duration}s, guidance: {guidance_scale}, seed: {seed}")
    
    with torch.no_grad():
        # Génération des latents avec inference_flow
        latents = model.inference_flow(
            prompt,
            duration=duration,
            num_inference_steps=steps,
            guidance_scale=guidance_scale,
            seed=seed,
        )
        
        # Transposer les latents
        latents_t = latents.transpose(2, 1)
        
        # Décodage direct
        with torch.amp.autocast('cuda', dtype=torch.float16):
            wave = vae.decode(latents_t).sample[0]
        
        # Déplacer vers CPU
        wave = wave.cpu()
        waveform_end = int(duration * 44100)
        wave = wave[:, :waveform_end]
        wave = wave.to(torch.float32)
        
        # Sauvegarder dans un fichier temporaire
        with tempfile.NamedTemporaryFile(suffix=".wav", delete=False) as f:
            temp_path = f.name
            torchaudio.save(temp_path, wave, sample_rate=44100)
    
    end_time = time.time()
    print(f"Génération terminée en {end_time - start_time:.2f} secondes")
    return temp_path

# Interface Gradio minimaliste mais suffisante pour l'API
with gr.Blocks(title="TangoFlux API") as demo:
    gr.Markdown("# TangoFlux API")
    gr.Markdown("Ce serveur expose uniquement l'API pour TangoFlux. Utilisez l'application cliente pour l'interface graphique.")
    
    # Interface minimale requise pour que l'API fonctionne
    # Ces composants ne sont pas affichés dans l'UI mais définissent les entrées/sorties de l'API
    with gr.Row(visible=False):  # On cache cette rangée
        prompt = gr.Textbox(value="large reverberant church stone walls")
        duration = gr.Number(value=5.0)  # Nous utilisons un Number au lieu d'un Slider
        steps = gr.Number(value=50)
        guidance_scale = gr.Number(value=3.5)
        seed = gr.Number(value=42)
        audio_output = gr.Audio()
        
        # Ce bouton n'est pas visible, mais définit l'endpoint API
        generate_btn = gr.Button("Générer")
        generate_btn.click(
            fn=generate_audio, 
            inputs=[prompt, duration, steps, guidance_scale, seed], 
            outputs=audio_output
        )

# Configurer l'API avec une file d'attente
demo.queue()
demo.launch(share=True)