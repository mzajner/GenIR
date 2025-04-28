import os
import shutil
import glob
from pathlib import Path
import torch
from PIL import Image
import re
from collections import Counter
from transformers import pipeline
import sys
import json
from datetime import datetime

def sanitize_filename(keyword):
    """Nettoie un mot-clé pour qu'il soit utilisable dans un nom de fichier"""
    keyword = re.sub(r'[^\w\s]', '', keyword)
    keyword = keyword.replace(' ', '')
    return keyword

def is_blueprint_or_diagram(image_path):
    """Détecte si une image est un plan ou un diagramme technique"""
    try:
        # 1. Analyse basée sur l'histogramme
        image = Image.open(image_path)
        image = image.convert('L')
        hist = image.histogram()
        
        sorted_hist = sorted([(i, count) for i, count in enumerate(hist) if count > 0], 
                            key=lambda x: x[1], reverse=True)
        
        total_pixels = image.width * image.height
        top_three_sum = sum(count for _, count in sorted_hist[:3])
        
        # 2. Détection basée sur le nom du fichier
        filename = os.path.basename(image_path).lower()
        plan_keywords = ["plan", "diagram", "blueprint", "map", "layout", "floor", "scheme", "position"]
        has_plan_keyword = any(keyword in filename for keyword in plan_keywords)
        
        # Critères combinés: soit histogramme, soit nom de fichier
        is_plan = (top_three_sum / total_pixels) > 0.8 or has_plan_keyword
        
        return is_plan
    except Exception as e:
        print(f"ERREUR lors de l'analyse de l'image pour détection de plan: {image_path}")
        print(f"Détail: {e}")
        return False  # En cas d'erreur, on considère que ce n'est pas un plan

# Définition des catégories et termes acceptables
ACOUSTIC_CATEGORIES = {
    "materials": [
        "wooden", "wood", "concrete", "stone", "marble", "brick", "glass", 
        "carpet", "fabric", "textile", "metal", "steel", "aluminum", "plastic", 
        "ceramic", "tile", "plaster", "drywall", "painted", "hardwood", 
        "softwood", "granite", "limestone", "slate", "clay", "velvet", "leather",
        "cork", "foam", "fiberglass", "wool", "cotton", "silk", "velour", "canvas",
        "grass", "rock", "mound", "leather", "brass", "bronze", "copper", "gold", 
        "silver", "mahogany", "oak", "pine", "walnut", "cedar", "bamboo", "vinyl",
        "linoleum", "porcelain", "terracotta", "sandstone", "quartz", "travertine",
        "wicker", "rattan", "suede", "nylon", "acrylic", "rubber", "chrome", "iron",
        "stucco", "crystal", "terrazzo", "resin", "formica", "teak", "maple", "birch",
        "ash", "elm", "cherry", "spruce", "redwood", "ebony", "rosewood", "hickory",
        "beech", "poplar", "sycamore", "balsa", "composite", "veneer", "laminate",
        "gypsum", "basalt", "marble", "onyx", "jade", "alabaster", "obsidian", "coral"
    ],
    
    "spatial": [
        # Termes de taille
        "large", "small", "huge", "tiny", "medium", "spacious", "intimate", 
        "expansive", "compact", "vast", "confined", "open", "enclosed", "enormous",
        "grand", "immense", "massive", "monumental", "colossal", "gigantic", "miniature",
        "cramped", "snug", "limited", "restricted", "extensive", "commodious", "ample",
        "capacious", "voluminous", "substantial", "generous", "copious", "abundant",
        "constrained", "modest", "diminutive", "slight", "petite", "undersized", "oversized",
        
        # Termes de forme
        "rectangular", "square", "circular", "oval", "curved", "angular", 
        "symmetrical", "asymmetrical", "hexagonal", "octagonal", "triangular", "rounded",
        "spherical", "cylindrical", "conical", "pyramidal", "cubical", "elliptical",
        "irregular", "organic", "geometric", "pentagonal", "diamond-shaped", "trapezoidal",
        "rhomboidal", "crescent", "spiral", "undulating", "serpentine", "zigzag", "sinuous",
        "wavy", "linear", "fluid", "rigid", "tapered", "pointed", "domed", "arched", "vaulted",
        
        # Termes de dimensions
        "high", "low", "tall", "short", "vaulted", "domed", "sloped", "flat", 
        "cathedral", "raised", "elevated", "lofty", "towering", "soaring", "stately",
        "imposing", "majestic", "squat", "stunted", "humble", "sunken", "depressed",
        "wide", "narrow", "broad", "constricted", "ample", "limited", "restricted",
        "sprawling", "extended", "stretched", "compressed", "squeezed", "roomy",
        "tight", "slim", "expansive", "generous", "confined", "constricted", "pinched",
        "cavernous", "spacious", "cramped", "proportional", "balanced", "elongated",
        "compressed", "stretched", "grand", "cozy", "intimate", "oversized", "undersized",
        "massive", "sweeping", "compressed", "vast", "constrained"
    ],
    
    "architecture": [
        "pillar", "column", "arch", "balcony", "stage", "steps", "stairs",
        "alcove", "niche", "beam", "vault", "dome", "cupola", "skylight", 
        "atrium", "corridor", "hallway", "arcade", "colonnade", "portico", "buttress",
        "cornice", "frieze", "pediment", "gable", "truss", "lintel", "capital", "plinth",
        "soffit", "fascia", "architrave", "molding", "coping", "eave", "quoin", "jamb",
        "mullion", "transom", "parapet", "turret", "spire", "steeple", "belfry", "nave",
        "transept", "apse", "chancel", "crypt", "vestibule", "foyer", "entryway", "loggia"
    ],
    
    "place_type": [
        "hall", "chamber", "studio", "auditorium", "theater", "church",
        "cathedral", "chapel", "concert", "lobby", "foyer", "atrium", "entryway",
        "stairwell", "gallery", "sanctuary", "temple", "mosque", "synagogue", 
        "monastery", "cloister", "office", "library", "classroom", "museum", 
        "parlor", "living", "dining", "kitchen", "bathroom", "basement", "cellar",
        "venue", "arena", "stadium", "gymnasium", "conference", "meeting", "lecture",
        "cave", "tunnel", "mound", "passage", "chamber", "courtroom", "court",
        "amphitheater", "opera", "conservatory", "recital", "pavilion", "rotunda",
        "sanctuary", "vestry", "chancel", "nave", "transept", "apse", "crypt",
        "refectory", "chapter", "dormitory", "abbey", "priory", "convent",
        # Ajout des termes inside/outside
        "inside", "outside",
        # Ajout de descripteurs d'environnements extérieurs
        "garden", "park", "courtyard", "square", "plaza", "street", "avenue", 
        "boulevard", "alley", "field", "forest", "woods", "meadow", "beach", 
        "shore", "coast", "mountain", "hill", "valley", "canyon", "desert", 
        "landscape", "yard", "patio", "terrace", "balcony", "rooftop", "outdoor", 
        "open-air", "waterfront", "lakeside", "riverside", "seaside", "playground", 
        "stadium", "arena", "amphitheater", "pavilion", "gazebo", "pergola", 
        "pathway", "trail", "bridge", "tunnel", "underpass", "overpass", "crosswalk",
        # Ajout de termes composés pour des types d'espaces
        "conference-room", "dining-room", "living-room", "board-room", "class-room",
        "waiting-room", "reading-room", "work-room", "prayer-room", "change-room",
        "rest-room", "bed-room", "bath-room", "hotel-room", "hospital-room",
        "lecture-hall", "meeting-room", "concert-hall", "music-room", "recording-studio"
    ],
    
    "acoustic_properties": [
        "reverberant", "dry", "damped", "live", "dead", "bright", "warm", "dark", 
        "balanced", "diffuse", "resonant", "absorptive", "reflective", "focused", 
        "scattered", "natural", "acoustic", "sonorous", "harmonic", "clear", "muffled",
        "sharp", "dull", "echoing", "ambient", "intimate", "immediate", "distant",
        "enveloping", "transparent", "defined", "articulate", "muddy", "boomy", "thin",
        "lush", "rich", "vibrant", "responsive", "reactive", "dampened", "attenuated",
        "filtered", "projection", "brilliant", "harsh", "smooth", "crisp", "blended",
        "separated", "flat", "colorful", "neutral", "pure", "distorted", "full", "hollow",
        "spacious", "tight", "open", "closed", "direct", "indirect", "airy", "weighty"
    ]
}

# Liste des mots à exclure explicitement
EXCLUDED_TERMS = [
    # Personnes
    "person", "people", "human", "man", "woman", "child", "audience", "crowd", "group",
    "spectator", "visitor", "tourist", "guest", "attendee", "occupant", "resident",
    
    # Éléments architecturaux à exclure
    "window", "door", "ceiling", "floor", "wall", "panel", "corner",
    
    # Termes acoustiques trop génériques ou ambigus à exclure
    "dark", "full",
    
    # Équipement audio
    "microphone", "mic", "speaker", "speakers", "amplifier", "amp", "mixer", "console", 
    "equipment", "audio", "sound", "recording", "playback", "stereo", "monitor", 
    "headphone", "earphone", "subwoofer", "woofer", "tweeter", "receiver", "deck",
    "system", "setup", "gear", "cable", "wire", "cord", "jack", "plug", "socket",
    
    # Instruments
    "instrument", "musical", "piano", "guitar", "drum", "violin", "cello", "bass", 
    "trumpet", "saxophone", "clarinet", "flute", "harp", "organ", "keyboard", "percussion",
    "string", "wind", "brass", "woodwind", "synthesizer", "electric", "acoustic",
    
    # Mobilier et équipement
    "chair", "chairs", "table", "desk", "bench", "sofa", "couch", "stool", "seat", 
    "furniture", "cabinet", "shelf", "bookcase", "stand", "podium", "lectern", "pulpit",
    "platform", "stage", "rostrum", "tripod", "lamp", "light", "curtain", "drape",
    
    # Électronique et équipement
    "laptop", "computer", "screen", "monitor", "projector", "camera", "video", "tv", 
    "television", "phone", "smartphone", "device", "gadget", "apparatus", "machine",
    "controller", "remote", "keyboard", "mouse", "tablet", "ipad", "display", "panel",
    
    # Art et décoration
    "painting", "picture", "artwork", "sculpture", "statue", "installation", "exhibit", 
    "display", "showcase", "frame", "poster", "print", "portrait", "landscape", "abstract",
    "photograph", "tapestry", "banner", "sign", "decoration", "ornament", "artifact",
    
    # Divers
    "event", "performance", "show", "concert", "recital", "lecture", "talk", "presentation", 
    "speech", "meeting", "conference", "convention", "workshop", "seminar", "class",
    "lesson", "course", "program", "activity", "function", "ceremony", "service", "ritual",
    "celebration", "party", "reception", "gala", "festival", "fair", "exposition", "plan",
    "drawing", "design", "blueprint", "sketch", "layout", "illustration", "diagram", "map",
    
    # Termes spécifiques évoqués dans votre exemple
    "windows", "building", "apodium", "andapainting", "apiano", "aspeaker", "alaptop", 
    "andaspeaker", "atripod",
    
    # Couleurs et termes visuels non pertinents pour l'acoustique
    "red", "blue", "green", "yellow", "orange", "purple", "pink", 
    "brown", "black", "white", "gray", "grey", "cyan", "magenta",
    "turquoise", "violet", "indigo", "maroon", "olive", "navy", "teal",
    "color", "colour", "colored", "coloured", "bright", "dark", "light",
    "shiny", "glossy", "matte", "transparent", "opaque", "translucent",
    "painted", "patterned", "textured", "smooth", "rough", "decorated"
]

# Paires de termes contradictoires
CONTRADICTORY_PAIRS = [
    ("small", "large"),
    ("high", "low"),
    ("wide", "narrow"),
    ("tall", "short"),
    ("open", "enclosed"),
    ("bright", "dark"),
    ("modern", "historic"),
    ("spacious", "confined"),
    ("vast", "compact"),
    ("ancient", "modern"),
    ("contemporary", "traditional"),
    ("dry", "reverberant"),
    # Ajout de la nouvelle paire contradictoire
    ("inside", "outside")
]

# Paires de synonymes à ne pas dupliquer
SYNONYM_PAIRS = [
    ("wood", "wooden"),
    ("concrete", "cement"),
    ("stone", "rocky"),
    ("metal", "metallic"),
    ("fabric", "textile"),
    ("circular", "curved", "rounded"),
    ("theater", "theatre"),
    ("large", "big", "huge"),
    ("small", "tiny", "little"),
    ("reverberant", "echoey", "echoing"),
    ("glass", "glazed"),
    ("marble", "marbled")
]

# Définition des termes composés pour extract_compound_terms
COMPOUND_TERMS = {
    "place_type": [
        "conference room", "dining room", "living room", "board room", "class room",
        "waiting room", "reading room", "work room", "prayer room", "change room",
        "rest room", "bed room", "bath room", "hotel room", "hospital room",
        "lecture hall", "meeting room", "concert hall", "music room", "recording studio"
    ]
}

# Définir les prompts spécifiques par catégorie - STRINGS INDIVIDUELLES, pas de listes
CATEGORY_PROMPTS = {
    "general": "",  # Prompt vide pour description générale
    
    # Prompts pour les matériaux 
    "materials_1": "The main materials in this space are",
    "materials_2": "This space is primarily made of",
    "materials_3": "The surfaces in this room are made of",
    "materials_4": "The dominant materials visible here are",
    "materials_5": "What materials can you identify in this space?",
    
    # Prompts pour le type de lieu
    "place_type_1": "This place is a",
    "place_type_2": "This space functions as a",
    "place_type_3": "This environment is used as a",
    "place_type_4": "This location appears to be a",
    "place_type_5": "What type of venue or location is this?",
    
    # Prompts pour l'architecture
    "architecture_1": "Architectural elements include",
    "architecture_2": "Notable architectural features are",
    "architecture_3": "The architectural design shows",
    "architecture_4": "The construction details include",
    "architecture_5": "What architectural features are visible in this space?",
    
    # Prompts pour les propriétés acoustiques
    "acoustic_properties_1": "If you were to clap in this space, it would sound",
    "acoustic_properties_2": "The reverberation in this space is",
    "acoustic_properties_3": "This space would sound acoustically",
    "acoustic_properties_4": "Speaking in this space would sound",
    "acoustic_properties_5": "How would sound behave in this environment?",
    
    # Prompts pour la catégorie spatiale combinée
    "spatial_1": "Is this room large or small?",
    "spatial_2": "What shape is this space?",
    "spatial_3": "Describe if this space is large, small, wide, narrow, tall, or short",
    "spatial_4": "Is this space circular, rectangular, square, or other shape?",
    "spatial_5": "The dimensions of this space can be described as"
}

# Mapper les catégories aux prompts
CATEGORY_TO_PROMPTS = {
    "materials": ["materials_1", "materials_2", "materials_3", "materials_4", "materials_5"],
    "place_type": ["place_type_1", "place_type_2", "place_type_3", "place_type_4", "place_type_5"],
    "architecture": ["architecture_1", "architecture_2", "architecture_3", "architecture_4", "architecture_5"],
    "acoustic_properties": ["acoustic_properties_1", "acoustic_properties_2", "acoustic_properties_3", 
                           "acoustic_properties_4", "acoustic_properties_5"],
    "spatial": ["spatial_1", "spatial_2", "spatial_3", "spatial_4", "spatial_5"]
}

def filter_contradictory_keywords(keywords, frequency_counter):
    """Filtre les mots-clés contradictoires en gardant le plus fréquent"""
    result = keywords.copy()
    
    for term1, term2 in CONTRADICTORY_PAIRS:
        if term1 in result and term2 in result:
            # Vérifier lequel est le plus fréquent
            count1 = frequency_counter.get(term1, 0)
            count2 = frequency_counter.get(term2, 0)
            
            if count1 > count2:
                result.remove(term2)
            else:
                result.remove(term1)
    
    return result

def filter_synonyms(keywords, frequency_counter):
    """Filtre les synonymes en gardant le plus fréquent"""
    result = keywords.copy()
    
    # Pour chaque groupe de synonymes
    for synonym_group in SYNONYM_PAIRS:
        # Vérifier quels synonymes sont présents
        present_synonyms = [term for term in synonym_group if term in result]
        
        # S'il y a plus d'un synonyme présent
        if len(present_synonyms) > 1:
            # Trier par fréquence
            present_synonyms.sort(key=lambda x: frequency_counter.get(x, 0), reverse=True)
            
            # Garder seulement le plus fréquent
            kept_term = present_synonyms[0]
            print(f"Synonymes trouvés: {', '.join(present_synonyms)}. Conservation de '{kept_term}'")
            
            # Retirer les autres
            for term in present_synonyms[1:]:
                if term in result:
                    result.remove(term)
    
    return result

def qualify_generic_room(text):
    """Tente de qualifier une 'room' générique basée sur le contexte"""
    room_types = {
        "meeting": ["meeting", "conference", "discussion", "board", "briefing"],
        "class": ["class", "lecture", "teaching", "educational", "school", "university", "classroom", "student"],
        "living": ["living", "sitting", "lounge", "residential", "home", "apartment", "cozy", "family"],
        "dining": ["dining", "eating", "food", "meal", "restaurant", "cafeteria", "breakfast", "lunch", "dinner"],
        "bed": ["bed", "sleeping", "dormitory", "hotel", "sleep", "rest", "nap"],
        "work": ["work", "office", "business", "professional", "corporate", "commercial", "workspace"],
        "waiting": ["waiting", "reception", "lobby", "entrance", "foyer"],
        "storage": ["storage", "storing", "closet", "utility", "equipment"],
        "bath": ["bath", "shower", "toilet", "sink", "washroom", "restroom"],
        "rehearsal": ["rehearsal", "practice", "music", "band", "orchestra", "choir", "ensemble"],
        "reading": ["reading", "library", "book", "study", "quiet", "books"],
        "game": ["game", "play", "gaming", "entertainment", "recreation"]
    }
    
    # Si le texte contient "room" mais n'a pas de type de room spécifique
    if " room" in text.lower() and not any(rt+"-room" in text.lower() or rt+" room" in text.lower() for rt in room_types):
        # Chercher des mots clés pour qualifier le type de pièce
        for room_type, keywords in room_types.items():
            if any(keyword in text.lower() for keyword in keywords):
                return f"{room_type}-room"
    
    return None

def split_compound_words(text):
    """Sépare les mots composés comme 'andaspeaker' en mots individuels"""
    # Modèles courants de préfixes
    prefixes = ['anda', 'a', 'the', 'with', 'and', 'of', 'in', 'on', 'at', 'by', 'to', 'for']
    
    # Pour chaque mot dans le texte
    words = re.findall(r'\b\w+\b', text.lower())
    result_words = []
    
    for word in words:
        # Vérifier si le mot commence par un préfixe connu
        found_prefix = False
        for prefix in sorted(prefixes, key=len, reverse=True):
            if word.startswith(prefix) and len(word) > len(prefix) + 2:  # +2 pour éviter des fragments trop courts
                remainder = word[len(prefix):]
                # Vérifier si le reste est un mot valide ou à exclure
                if remainder in ALL_VALID_TERMS or remainder in EXCLUDED_TERMS:
                    # On garde juste la partie valide mais sans le préfixe
                    result_words.append(remainder)
                    found_prefix = True
                    break
        
        if not found_prefix:
            result_words.append(word)
    
    return result_words

def extract_compound_terms(text, category):
    """Extrait des termes composés comme 'conference room' à partir d'un texte"""
    compound_terms = []
    
    if category in COMPOUND_TERMS:
        for term in COMPOUND_TERMS[category]:
            if term.lower() in text.lower():
                # Créer une version avec tiret pour le nom de fichier
                sanitized_term = term.replace(" ", "-")
                compound_terms.append(sanitized_term)
                print(f"Terme composé trouvé: '{term}' -> '{sanitized_term}'")
    
    return compound_terms

def extract_category_keywords(text, category):
    """Extrait les mots-clés d'une catégorie spécifique à partir d'un texte"""
    # Diviser le texte en mots
    words = re.findall(r'\b\w+\b', text.lower())
    
    # Traiter les mots composés
    words = split_compound_words(' '.join(words))
    
    # Filtrer les mots de liaison et très courts
    stop_words = ['the', 'a', 'an', 'and', 'is', 'are', 'in', 'on', 'at', 'with', 
                 'from', 'to', 'of', 'for', 'by', 'as', 'it', 'its', 'this', 'that', 
                 'has', 'have', 'had', 'can', 'will', 'would', 'could', 'should',
                 # Ajouter d'autres mots problématiques
                 'like', 'made', 'make', 'making', 'looks', 'appears', 'seems', 
                 'seen', 'looks', 'looked', 'looking', 'see', 'saw', 'seen',
                 'shows', 'showing', 'shown', 'shown', 'displayed', 'displaying',
                 'featured', 'featuring', 'includes', 'including', 'included',
                 'contains', 'containing', 'contained', 'holds', 'holding', 'held',
                 'was', 'were', 'being', 'been', 'am', 'used', 'using', 'use',
                 'utilizing', 'utilize', 'utilized', 'employing', 'employ', 'employed']
    words = [word for word in words if word not in stop_words and len(word) > 2]
    
    # Filtrer les mots exclus
    words = [word for word in words if word not in EXCLUDED_TERMS]
    
    # Récupérer UNIQUEMENT les mots qui appartiennent à la catégorie
    # Cette ligne est critique - elle ne devrait permettre que les mots dans le dictionnaire
    valid_words = []
    for word in words:
        if word in ACOUSTIC_CATEGORIES[category]:
            valid_words.append(word)
    
    # Pour la catégorie place_type, traitement additionnel
    if category == "place_type":
        # Rechercher des termes composés
        compound_words = extract_compound_terms(text, category)
        valid_words.extend(compound_words)
        
        # Tenter de qualifier une room générique
        qualified_room = qualify_generic_room(text)
        if qualified_room and qualified_room not in valid_words:
            valid_words.append(qualified_room)
            print(f"Room générique qualifiée: '{qualified_room}'")
    
    return valid_words

def extract_spatial_keywords(text):
    """Fonction spécialisée pour extraire les caractéristiques spatiales"""
    # Rechercher explicitement des mots-clés spatiaux importants
    size_keywords = {
        "large": ["large", "big", "huge", "enormous", "vast", "spacious", "expansive", "grand"],
        "small": ["small", "tiny", "little", "compact", "cramped", "narrow", "tight", "constrained"],
        "high": ["high", "tall", "lofty", "towering", "soaring", "elevated", "vaulted"],
        "low": ["low", "short", "squat", "stunted", "sunken", "depressed"],
        "wide": ["wide", "broad", "ample", "generous", "expansive", "roomy"],
        "narrow": ["narrow", "thin", "slim", "constricted", "restricted", "limited"]
    }
    
    shape_keywords = {
        "rectangular": ["rectangular", "rectangle", "oblong", "box-shaped", "boxy"],
        "square": ["square", "boxy", "cube-shaped", "cubic"],
        "circular": ["circular", "circle", "round", "rounded", "curved", "oval"],
        "curved": ["curved", "curved shape", "curvature", "arc", "arcing", "arched"],
        "triangular": ["triangular", "triangle", "pyramid-shaped", "pyramid", "conical"],
        "irregular": ["irregular", "asymmetric", "uneven", "non-uniform", "organic"],
        "domed": ["domed", "dome", "cupola", "hemispherical", "half-sphere"],
        "arched": ["arched", "arch", "vaulted", "vault", "curved ceiling"],
        "angular": ["angular", "angled", "cornered", "edged", "sharp-edged"]
    }
    
    # Normaliser le texte
    text_lower = text.lower()
    
    # Rechercher les mots-clés de taille
    found_size = []
    for key, synonyms in size_keywords.items():
        if any(synonym in text_lower for synonym in synonyms):
            found_size.append(key)
    
    # Rechercher les mots-clés de forme
    found_shape = []
    for key, synonyms in shape_keywords.items():
        if any(synonym in text_lower for synonym in synonyms):
            found_shape.append(key)
    
    # Rechercher des indices directs sur la taille
    if "large" not in found_size and "small" not in found_size:
        if re.search(r'(it|space|room|area|place) is (quite |very |extremely |really |pretty )?(big|large|huge|spacious)', text_lower):
            found_size.append("large")
        elif re.search(r'(it|space|room|area|place) is (quite |very |extremely |really |pretty )?(small|tiny|little|compact)', text_lower):
            found_size.append("small")
    
    # Rechercher des indices directs sur la forme
    if not found_shape:
        if re.search(r'(it|space|room|area|place) is (quite |very |extremely |really |pretty )?(round|circular|curved|rounded)', text_lower):
            found_shape.append("circular")
        elif re.search(r'(it|space|room|area|place) is (quite |very |extremely |really |pretty )?(rectangular|square)', text_lower):
            found_shape.append("rectangular")
    
    # Rechercher des mots-clés spatiaux spécifiques dans le texte
    spatial_words = re.findall(r'\b(large|small|big|huge|tiny|spacious|vast|cramped|tight|narrow|wide|tall|short|high|low|circular|rectangular|square|curved|rounded|angular|triangular|irregular|symmetric|asymmetric|domed|arched|vaulted)\b', text_lower)
    
    # Filtrer pour ne garder que les mots dans ACOUSTIC_CATEGORIES['spatial']
    valid_spatial_words = [word for word in spatial_words if word in ACOUSTIC_CATEGORIES['spatial']]
    
    # Combiner les résultats
    all_spatial = found_size + found_shape + valid_spatial_words
    
    # Éliminer les doublons tout en préservant l'ordre
    unique_spatial = []
    for word in all_spatial:
        if word not in unique_spatial:
            unique_spatial.append(word)
    
    return unique_spatial

def improve_blip_generation(image, prompt, pipe):
    """Améliore la génération BLIP avec des techniques de fallback et un nettoyage amélioré"""
    # Essai initial
    try:
        result = pipe(image, prompt=prompt, max_new_tokens=30)[0]['generated_text']
        
        # Créer une version en minuscules pour les comparaisons insensibles à la casse
        result_lower = result.lower()
        prompt_lower = prompt.lower()
        
        # Vérifier différentes façons dont le prompt pourrait être inclus dans la réponse
        prompt_included = False
        cleaned_result = result
        
        # 1. Vérifier si la réponse commence exactement par le prompt
        if result_lower.startswith(prompt_lower):
            prompt_included = True
            cleaned_result = result[len(prompt):].strip()
        
        # 2. Vérifier pour des fragments de question courants au début
        question_fragments = ["describe the ", "what is the ", "how would you ", "can you describe ", 
                             "tell me about ", "?", "visible in this image is", "describe this", 
                             "the space?", "this space", "the room?", "this room", "overall spatial",
                             "the spatial layout", "this space can be", "the volume", "is this space"]
        
        for fragment in question_fragments:
            if fragment in result_lower[:60]:  # Recherche sur plus de caractères
                prompt_included = True
                # Trouver où le fragment se termine dans la réponse
                fragment_end = result_lower.find(fragment) + len(fragment)
                # S'assurer que nous n'allons pas trop loin
                if fragment_end < len(result):
                    cleaned_result = result[fragment_end:].strip()
                break
        
        # 3. Chercher le premier signe de ponctuation qui pourrait indiquer la fin d'une question
        if not prompt_included and ("?" in result_lower[:40] or ":" in result_lower[:40]):
            question_end = max(result_lower.find("?"), result_lower.find(":"))
            if question_end > 0:
                prompt_included = True
                cleaned_result = result[question_end+1:].strip()
        
        # Filtrer les réponses génériques non-informatives
        generic_responses = ["very interesting", "a work of art", "i'm not sure", "i can't tell",
                            "i don't know", "it's hard to say", "it's difficult to determine",
                            "it's not clear", "it depends", "it varies", "cannot be determined"]
        
        for generic in generic_responses:
            if generic in cleaned_result.lower() and len(cleaned_result) < 30:
                cleaned_result = ""  # Considérer comme une réponse vide
                break
        
        # Si le prompt semble toujours inclus même après les tentatives ci-dessus
        # OU si la réponse est vide ou très courte
        if prompt_included and len(cleaned_result) < 10 or not cleaned_result:
            # Essayer avec un prompt complètement différent
            if "spatial" in prompt_lower or "shape" in prompt_lower or "size" in prompt_lower:
                # Prompts spécifiques pour la catégorie spatiale
                alt_prompts = [
                    "Is this space large or small?",
                    "What shape is this room?",
                    "Is this area wide or narrow?",
                    "Would you describe this as a big space?",
                    "Is this a rectangular, circular, or irregular space?"
                ]
                import random
                alt_prompt = random.choice(alt_prompts)
            else:
                alt_prompt = "Describe what you see in this image"
                
            alt_result = pipe(image, prompt=alt_prompt, max_new_tokens=50)[0]['generated_text']
            
            # Éviter la récursion infinie en ne rappelant pas la fonction
            # Nettoyer simplement le résultat alternatif
            if alt_result.lower().startswith(alt_prompt.lower()):
                cleaned_result = alt_result[len(alt_prompt):].strip()
            else:
                cleaned_result = alt_result
        
        # Nettoyage final pour éliminer les caractères de ponctuation au début
        cleaned_result = re.sub(r'^[.,:;\s]+', '', cleaned_result)
        
        # Si après tous ces nettoyages, la réponse est toujours trop courte
        if len(cleaned_result) < 5:
            # Dernier recours: générer sans prompt
            no_prompt_result = pipe(image, max_new_tokens=50)[0]['generated_text']
            return no_prompt_result
            
        return cleaned_result
    
    except Exception as e:
        print(f"Erreur lors de la génération améliorée: {e}")
        # En cas d'échec, générer une description générique
        try:
            return pipe(image, max_new_tokens=40)[0]['generated_text']
        except:
            return "No description available"

def analyze_folder_context(folder_path):
    """Analyse le chemin du dossier pour extraire des informations contextuelles"""
    # Normaliser le chemin
    normalized_path = folder_path.replace("\\", "/").lower()
    path_parts = normalized_path.split("/")
    
    # Filtrer pour ne garder que les parties significatives du chemin (éliminer 'desktop', 'images', etc.)
    non_informative_parts = ['desktop', 'images', 'c:', 'users', 'documents', 'downloads', 'irs', 'gratuites', 'achetees']
    filtered_parts = [part for part in path_parts if part.lower() not in non_informative_parts]
    
    # Diviser les parties du chemin en sous-parties (pour gérer les cas comme "york-guildhall-council-chamber")
    subparts = []
    for part in filtered_parts:
        # Diviser sur les tirets et soulignés
        parts = re.split(r'[-_]', part)
        # Ajouter à la fois la partie complète et ses sous-parties
        subparts.append(part)
        subparts.extend(parts)
    
    # Dictionnaire des indices contextuels
    contextual_hints = {
        "inferred_categories": {},
        "confidence": {}
    }
    
    # Mots-clés contextuels par catégorie
    context_keywords = {
        "place_type": {
            "church": ["church", "cathedral", "chapel", "sanctuary", "altar", "nave"],
            "hall": ["hall", "auditorium", "theater", "theatre", "venue", "stage"],
            "studio": ["studio", "recording", "booth", "session", "mix"],
            "bathroom": ["bathroom", "shower", "toilet", "lavatory", "bath"],
            "outdoor": ["outdoor", "garden", "park", "field", "forest", "beach", "nature"],
            "conference-room": ["conference", "council", "meeting", "chamber", "boardroom"],
            "concert-hall": ["concert", "music", "acoustic", "performance"],
            "living-room": ["living", "residential", "home", "apartment", "house"],
            "classroom": ["class", "school", "education", "lecture", "teaching"],
            "dining-room": ["dining", "restaurant", "cafeteria", "cafe", "canteen"]
        },
        "acoustic_properties": {
            "reverberant": ["cathedral", "church", "hall", "auditorium", "theater", "cave"],
            "dry": ["studio", "booth", "bedroom", "office", "recording"],
            "live": ["concert", "performance", "stage", "live"],
            "bright": ["tile", "marble", "glass", "reflective"],
            "damped": ["bedroom", "office", "studio"]
        },
        "materials": {
            "wood": ["wooden", "timber", "oak", "pine", "hardwood"],
            "stone": ["marble", "granite", "slate", "brick", "concrete"],
            "glass": ["mirror", "window", "glazed", "pane"],
            "fabric": ["carpet", "upholstered", "textile", "cloth"]
        }
    }
    
    # Table de conversion pour les termes composés fréquents dans les noms de dossier
    compound_folder_terms = {
        "council-chamber": "conference-room",
        "meeting-room": "conference-room",
        "guildhall": "hall",
        "concert-hall": "concert-hall",
        "dining-hall": "dining-room",
        "lecture-hall": "classroom",
        "recording-studio": "studio",
        "concert-room": "concert-hall",
        "church-hall": "church"
    }
    
    print("\nAnalyse du contexte du chemin d'accès:")
    
    # Vérification directe des termes dans ACOUSTIC_CATEGORIES
    found_direct_matches = False
    
    # Vérifier si des termes apparaissent directement dans les catégories
    for category, terms in ACOUSTIC_CATEGORIES.items():
        for part in subparts:
            if part in terms:
                if part not in contextual_hints["inferred_categories"]:
                    contextual_hints["inferred_categories"][part] = []
                
                contextual_hints["inferred_categories"][part].append(category)
                
                if part not in contextual_hints["confidence"]:
                    contextual_hints["confidence"][part] = 0
                # Donner une confiance élevée aux correspondances directes
                contextual_hints["confidence"][part] += 5
                found_direct_matches = True
                print(f"  - Correspondance directe trouvée: '{part}' [{category}]")
    
    # Vérifier les termes composés spécifiques dans les noms de dossier
    for part in subparts:
        for compound_term, mapped_term in compound_folder_terms.items():
            if compound_term in part or all(word in part for word in compound_term.split("-")):
                if mapped_term not in contextual_hints["inferred_categories"]:
                    contextual_hints["inferred_categories"][mapped_term] = []
                
                contextual_hints["inferred_categories"][mapped_term].append("place_type")
                
                if mapped_term not in contextual_hints["confidence"]:
                    contextual_hints["confidence"][mapped_term] = 0
                # Donner une confiance élevée aux correspondances de termes composés
                contextual_hints["confidence"][mapped_term] += 4
                print(f"  - Terme composé trouvé: '{part}' -> '{mapped_term}' [place_type]")
    
    # Analyser chaque partie/sous-partie du chemin pour des indices contextuels 
    # en utilisant la table de recherche context_keywords
    for part in subparts:
        # Mots du nom de dossier
        words = re.findall(r'\b[a-z]{3,}\b', part)
        
        for category, category_keywords in context_keywords.items():
            for keyword, related_terms in category_keywords.items():
                # Vérifier si des termes associés apparaissent
                for word in words:
                    if word in related_terms or word == keyword:
                        if keyword not in contextual_hints["inferred_categories"]:
                            contextual_hints["inferred_categories"][keyword] = []
                        
                        contextual_hints["inferred_categories"][keyword].append(category)
                        
                        # Augmenter la confiance pour ce mot-clé
                        if keyword not in contextual_hints["confidence"]:
                            contextual_hints["confidence"][keyword] = 0
                        contextual_hints["confidence"][keyword] += 1
                        
                        print(f"  - Indice contextuel trouvé: '{word}' -> '{keyword}' [{category}]")
    
    # Filtrer les résultats par niveau de confiance
    high_confidence_hints = {}
    for keyword, confidence in contextual_hints["confidence"].items():
        if confidence >= 1:  # Seuil minimal pour considérer un indice comme valide
            categories = contextual_hints["inferred_categories"][keyword]
            # Prendre la catégorie la plus fréquente
            category_counts = Counter(categories)
            most_common_category = category_counts.most_common(1)[0][0]
            
            if most_common_category not in high_confidence_hints:
                high_confidence_hints[most_common_category] = []
            high_confidence_hints[most_common_category].append(keyword)
    
    if high_confidence_hints:
        print("\nIndices contextuels retenus du chemin d'accès:")
        for category, keywords in high_confidence_hints.items():
            print(f"  {category}: {', '.join(keywords)}")
    else:
        print("  Aucun indice contextuel significatif trouvé dans le chemin d'accès")
    
    return high_confidence_hints

def cleanup_output_folders(root_dir):
    """Supprime les dossiers de sortie des exécutions précédentes"""
    print("Nettoyage des dossiers de sortie précédents...")
    
    # Identifier tous les répertoires contenant "BFormatToStereo" ou "stereo"
    bformat_dirs = glob.glob(os.path.join(root_dir, '**/BFormatToStereo'), recursive=True)
    stereo_dirs = glob.glob(os.path.join(root_dir, '**/stereo'), recursive=True)
    
    # Pour chaque répertoire BFormatToStereo, vérifier et supprimer le dossier BFormatToStereoLabelled correspondant
    for bformat_dir in bformat_dirs:
        parent_dir = os.path.dirname(bformat_dir)
        labelled_dir = os.path.join(parent_dir, "BFormatToStereoLabelled")
        
        if os.path.exists(labelled_dir):
            try:
                print(f"Suppression du dossier: {labelled_dir}")
                shutil.rmtree(labelled_dir)
            except Exception as e:
                print(f"ATTENTION: Échec de suppression de {labelled_dir}: {e}")
    
    # Pour chaque répertoire stereo, vérifier et supprimer le dossier stereoLabelled correspondant
    for stereo_dir in stereo_dirs:
        parent_dir = os.path.dirname(stereo_dir)
        labelled_dir = os.path.join(parent_dir, "stereoLabelled")
        
        if os.path.exists(labelled_dir):
            try:
                print(f"Suppression du dossier: {labelled_dir}")
                shutil.rmtree(labelled_dir)
            except Exception as e:
                print(f"ATTENTION: Échec de suppression de {labelled_dir}: {e}")
    
    print("Nettoyage terminé.")

# Créer une liste de tous les termes valides pour faciliter la vérification
ALL_VALID_TERMS = []
for category in ACOUSTIC_CATEGORIES.values():
    ALL_VALID_TERMS.extend(category)

def extract_keywords_from_image(image_path):
    """Extrait des mots-clés descriptifs d'une image en utilisant BLIP"""
    # Définir les catégories à traiter
    categories_to_process = ["materials", "place_type", "architecture", "acoustic_properties", "spatial"]
    
    # Vérifier en amont si c'est un plan ou diagramme - maintenant plus robuste
    if is_blueprint_or_diagram(image_path):
        return [], {}  # Sortir immédiatement sans tenter d'analyser
    
    # Vérifier le texte de l'image pour des mots-clés de plan
    plan_keywords = ["floor plan", "blueprint", "diagram", "map", "layout", "schematic", "floor layout"]
    
    try:
        pipe = pipeline("image-to-text", 
                      model="Salesforce/blip-image-captioning-large", 
                      device=0 if torch.cuda.is_available() else -1)
        
        # Génération rapide pour vérifier si c'est un plan
        quick_desc = pipe(Image.open(image_path), max_new_tokens=30)[0]['generated_text'].lower()
        
        # Si la description contient des mots-clés de plan
        if any(keyword in quick_desc for keyword in plan_keywords):
            return [], {}  # C'est un plan, sortir
    except Exception as e:
        print(f"ERREUR CRITIQUE: Impossible de charger le modèle de reconnaissance d'image")
        print(f"Détail: {e}")
        raise
    
    try:
        image = Image.open(image_path)
        
        all_extracted_texts = []
        category_texts = {cat: [] for cat in ACOUSTIC_CATEGORIES.keys()}
        
        # Obtenir une description générale sans prompt
        general_result = pipe(image, max_new_tokens=50)[0]['generated_text']
        all_extracted_texts.append(general_result)
        print(f"Description générale: {general_result}")
        
        # Traiter chaque catégorie sélectionnée
        for category in categories_to_process:
            # Obtenir le premier prompt disponible pour cette catégorie
            if category in CATEGORY_TO_PROMPTS and len(CATEGORY_TO_PROMPTS[category]) > 0:
                prompt_key = CATEGORY_TO_PROMPTS[category][0]  # Utiliser le premier prompt pour stabilité
                
                if prompt_key in CATEGORY_PROMPTS:
                    prompt = CATEGORY_PROMPTS[prompt_key]  # Obtenir la chaîne de prompt
                    
                    # Utiliser la génération améliorée
                    cleaned_result = improve_blip_generation(image, prompt, pipe)
                    
                    if cleaned_result:
                        all_extracted_texts.append(cleaned_result)
                        category_texts[category].append(cleaned_result)
                        print(f"Catégorie {category}: {cleaned_result}")
        
        # Combiner tous les textes extraits
        full_text = ' '.join(all_extracted_texts)
        
        # Initialiser les mots-clés extraits pour chaque catégorie
        keywords_by_category = {category: [] for category in ACOUSTIC_CATEGORIES.keys()}
        
        # Essayer jusqu'à 10 fois pour chaque catégorie qui n'a pas de mots-clés
        for category in categories_to_process:
            # D'abord, extraire les mots-clés du texte déjà généré
            category_keywords = extract_category_keywords(full_text, category)
            
            # Pour la catégorie spatiale, utiliser notre extracteur spécialisé
            if category == "spatial" and not category_keywords:
                spatial_keywords = extract_spatial_keywords(full_text)
                if spatial_keywords:
                    category_keywords = spatial_keywords
            
            # Si nous avons déjà des mots-clés, les ajouter et continuer
            if category_keywords:
                keywords_by_category[category].extend(category_keywords)
                print(f"Mots-clés initiaux [{category}]: {', '.join(category_keywords)}")
                continue
                
            # Sinon, essayer jusqu'à 10 fois avec des prompts spécifiques
            attempts = 0
            max_attempts = 10
            
            # Obtenir les clés de prompts pour cette catégorie
            if category in CATEGORY_TO_PROMPTS:
                prompt_keys = CATEGORY_TO_PROMPTS[category]
            else:
                # Fallback vers un prompt générique simple
                generic_prompt = f"Describe the {category.replace('_', ' ')} of this space"
                temp_key = f"{category}_generic"
                CATEGORY_PROMPTS[temp_key] = generic_prompt
                prompt_keys = [temp_key]
            
            while attempts < max_attempts and not keywords_by_category[category]:
                # Sélectionner un prompt de manière séquentielle
                prompt_index = attempts % len(prompt_keys)
                current_prompt_key = prompt_keys[prompt_index]
                
                # Obtenir le prompt associé à cette clé
                if current_prompt_key in CATEGORY_PROMPTS:
                    current_prompt = CATEGORY_PROMPTS[current_prompt_key]
                    
                    try:
                        # Utiliser la génération améliorée au lieu de la simple
                        cleaned_result = improve_blip_generation(image, current_prompt, pipe)
                        
                        print(f"Catégorie {category} (tentative {attempts+1}): {cleaned_result}")
                        
                        # Extraire les mots-clés de cette nouvelle description
                        attempt_keywords = extract_category_keywords(cleaned_result, category)
                        
                        # Pour la catégorie spatiale, essayer l'extracteur spécialisé si nécessaire
                        if category == "spatial" and not attempt_keywords:
                            spatial_attempt_keywords = extract_spatial_keywords(cleaned_result)
                            if spatial_attempt_keywords:
                                attempt_keywords = spatial_attempt_keywords
                        
                        if attempt_keywords:
                            # On a trouvé des mots-clés
                            keywords_by_category[category].extend(attempt_keywords)
                            print(f"Mots-clés trouvés [{category}]: {', '.join(attempt_keywords)}")
                            break
                    except Exception as e:
                        print(f"Erreur lors de la génération: {e}")
                
                attempts += 1
            
            # Si on n'a toujours pas de mots-clés après toutes les tentatives
            if not keywords_by_category[category]:
                print(f"Aucun mot-clé trouvé pour [{category}] après {attempts} tentatives")
                
                # Essayer une dernière tentative avec un prompt ultra simple
                try:
                    if category == "spatial":
                        simple_prompt = "Is this room large or small?"
                    else:
                        simple_prompt = f"This {category.replace('_', ' ')} is"
                    
                    cleaned_result = improve_blip_generation(image, simple_prompt, pipe)
                    print(f"Tentative simplifiée pour {category}: {cleaned_result}")
                    
                    simple_keywords = extract_category_keywords(cleaned_result, category)
                    
                    # Pour la catégorie spatiale, essayer l'extracteur spécialisé si nécessaire
                    if category == "spatial" and not simple_keywords:
                        spatial_simple_keywords = extract_spatial_keywords(cleaned_result)
                        if spatial_simple_keywords:
                            simple_keywords = spatial_simple_keywords
                    
                    if simple_keywords:
                        keywords_by_category[category].extend(simple_keywords)
                        print(f"Mots-clés trouvés en secours [{category}]: {', '.join(simple_keywords)}")
                except Exception as e:
                    print(f"Erreur lors de la tentative simplifiée: {e}")
        
        # Afficher un résumé des mots-clés extraits par catégorie
        print("\nRÉSUMÉ DES MOTS-CLÉS EXTRAITS:")
        for category in categories_to_process:
            if keywords_by_category[category]:
                print(f"[{category}]: {', '.join(keywords_by_category[category])}")
            else:
                print(f"[{category}]: aucun mot-clé trouvé")
        
        # Aplatir la liste de mots-clés
        all_keywords = []
        for category in categories_to_process:
            all_keywords.extend(keywords_by_category[category])
        
        # Éliminer les doublons
        all_keywords = list(set(all_keywords))
        
        return all_keywords, keywords_by_category
        
    except Exception as e:
        print(f"ERREUR lors de l'analyse de {image_path}")
        print(f"Détail: {e}")
        return [], {}

def process_images_by_folder(root_dir, min_keywords=5, max_keywords=15):
    """Traite toutes les images regroupées par dossier"""
    # Définir les catégories à traiter
    categories_to_process = ["materials", "place_type", "architecture", "acoustic_properties", "spatial"]
    
    # Nettoyer les dossiers de sortie précédents avant de commencer
    cleanup_output_folders(root_dir)
    
    image_extensions = ['*.jpg', '*.jpeg', '*.png', '*.bmp']
    
    all_images = []
    for ext in image_extensions:
        all_images.extend(glob.glob(os.path.join(root_dir, '**', ext), recursive=True))
    
    print(f"Trouvé {len(all_images)} images à traiter")
    if len(all_images) == 0:
        print("AVERTISSEMENT: Aucune image trouvée. Vérifiez le chemin du répertoire.")
        return
    
    images_by_folder = {}
    for img_path in all_images:
        folder = os.path.dirname(img_path)
        if folder not in images_by_folder:
            images_by_folder[folder] = []
        images_by_folder[folder].append(img_path)
    
    print(f"\nTraitement de {len(images_by_folder)} dossiers...")
    
    folder_count = 0
    for folder, images in images_by_folder.items():
        folder_count += 1
        print(f"\n\n======= DOSSIER {folder_count}/{len(images_by_folder)} =======")
        print(f"Traitement du dossier: {folder}")
        print(f"Contient {len(images)} images")
        print("="*50)
        
        try:
            # Filtrer les plans et diagrammes en amont
            valid_images = []
            for img_path in images:
                if not is_blueprint_or_diagram(img_path):
                    valid_images.append(img_path)
            
            if not valid_images:
                continue
                
            # Analyser chaque image et collecter tous les mots-clés
            all_keywords = []
            folder_keywords_by_category = {category: [] for category in ACOUSTIC_CATEGORIES.keys()}
            
            for img_path in valid_images:
                try:
                    img_keywords, img_keywords_by_category = extract_keywords_from_image(img_path)
                    
                    if img_keywords:
                        all_keywords.extend(img_keywords)
                        
                        # Collecter également par catégorie
                        for category, keywords in img_keywords_by_category.items():
                            folder_keywords_by_category[category].extend(keywords)
                
                except Exception as e:
                    print(f"ATTENTION: Échec de l'extraction pour {os.path.basename(img_path)}: {e}")
            
            # Analyser le contexte du dossier
            contextual_hints = analyze_folder_context(folder)
            print("\nAnalyse du contexte du dossier...")
            
            # Utiliser les indices contextuels pour enrichir les mots-clés
            for category, hints in contextual_hints.items():
                for hint in hints:
                    if hint in ACOUSTIC_CATEGORIES[category] and hint not in folder_keywords_by_category[category]:
                        folder_keywords_by_category[category].append(hint)
                        all_keywords.append(hint)
                        print(f"Ajout du mot-clé contextuel: {hint} ({category})")
            
            # Si aucun mot-clé n'a été extrait
            if not all_keywords:
                print(f"AVERTISSEMENT: Aucun mot-clé n'a pu être extrait des images dans {folder}")
                # Utiliser "NoLabel" à la place des mots-clés par défaut
                all_keywords = ["NoLabel"] * min_keywords
                for category in folder_keywords_by_category:
                    folder_keywords_by_category[category].append("NoLabel")
            
            # Compter la fréquence des mots-clés par catégorie
            category_counters = {}
            for category, keywords in folder_keywords_by_category.items():
                category_counters[category] = Counter(keywords)
            
            # Afficher un résumé de tous les mots-clés trouvés par catégorie
            print("\nMOTS-CLÉS TROUVÉS PAR CATÉGORIE:")
            for category in categories_to_process:
                keywords_with_freq = [(kw, freq) for kw, freq in category_counters[category].items()]
                keywords_with_freq.sort(key=lambda x: x[1], reverse=True)
                
                if keywords_with_freq:
                    formatted_keywords = [f"{kw} (×{freq})" for kw, freq in keywords_with_freq]
                    print(f"[{category}]: {', '.join(formatted_keywords)}")
                else:
                    print(f"[{category}]: aucun mot-clé trouvé")
            
            # Sélectionner les mots-clés finaux
            selected_keywords = []
            
            # 1. D'abord, sélectionner les matériaux les plus fréquents (jusqu'à 6)
            materials_with_freq = [(mat, freq) for mat, freq in category_counters['materials'].items() if freq >= 2]
            materials_with_freq.sort(key=lambda x: x[1], reverse=True)
            selected_materials = [mat for mat, _ in materials_with_freq[:6]]
            selected_keywords.extend(selected_materials)
            
            # 2. Ensuite, sélectionner jusqu'à 2 types de lieu (sans valeurs par défaut)
            place_types_with_freq = [(place, freq) for place, freq in category_counters['place_type'].items() if freq >= 2]
            place_types_with_freq.sort(key=lambda x: x[1], reverse=True)
            selected_place_types = [place for place, _ in place_types_with_freq[:2]]
            
            # Si nous n'avons pas 2 types de lieu, compléter avec les plus fréquents même s'ils apparaissent une seule fois
            if len(selected_place_types) < 2:
                remaining_place_types = [(place, freq) for place, freq in category_counters['place_type'].items() 
                                        if freq == 1 and place not in selected_place_types]
                remaining_place_types.sort(key=lambda x: x[1], reverse=True)
                selected_place_types.extend([place for place, _ in remaining_place_types[:2-len(selected_place_types)]])
                
            # Ajouter à la liste des mots-clés sélectionnés
            selected_keywords.extend(selected_place_types)
            
            # Vérifier si nous avons des propriétés acoustiques
            if not any(folder_keywords_by_category['acoustic_properties']):
                # Inférer des propriétés acoustiques à partir des autres catégories
                # Définir les propriétés acoustiques principales
                core_acoustic_properties = {
                    "reverberant": "Fort écho et longue durée de réverbération",
                    "dry": "Très peu de réverbération",
                    "bright": "Riche en hautes fréquences",
                    "warm": "Riche en basses/moyennes fréquences",
                    "balanced": "Répartition équilibrée des fréquences",
                    "diffuse": "Réverbération dispersée uniformément",
                    "resonant": "Certaines fréquences sont amplifiées",
                    "intimate": "Proximité perçue de la source sonore",
                    "spacious": "Impression d'espace et de profondeur",
                    "clear": "Bonne intelligibilité et définition",
                    "harsh": "Agressif dans les hautes fréquences",
                    "dead": "Absence de réflexions et de vie",
                    "live": "Beaucoup de réflexions, espace vivant",
                    "focused": "Réflexions dirigées/concentrées"
                }
                
                # Table d'association entre caractéristiques et propriétés acoustiques
                acoustic_mapping = {
                    # Types de lieux
                    "church": ["reverberant", "resonant", "spacious"],
                    "cathedral": ["reverberant", "resonant", "spacious"],
                    "chapel": ["reverberant", "intimate"],
                    "hall": ["reverberant", "spacious", "clear"],
                    "concert": ["balanced", "clear", "diffuse"],
                    "theater": ["balanced", "clear", "diffuse"],
                    "auditorium": ["balanced", "clear", "diffuse"],
                    "studio": ["dry", "balanced", "dead"],
                    "bedroom": ["dry", "intimate", "warm"],
                    "bathroom": ["bright", "live", "resonant"],
                    "living": ["balanced", "warm", "intimate"],
                    "office": ["dry", "balanced", "dead"],
                    "library": ["dry", "dead", "intimate"],
                    "classroom": ["dry", "clear", "balanced"],
                    "opera": ["balanced", "clear", "resonant"],
                    "cave": ["reverberant", "resonant", "focused"],
                    "tunnel": ["reverberant", "focused", "resonant"],
                    "outside": ["dry", "clear", "diffuse"],
                    "garden": ["dry", "diffuse", "clear"],
                    "park": ["dry", "diffuse", "clear"],
                    "forest": ["diffuse", "absorptive", "intimate"],
                    "conference-room": ["dry", "clear", "balanced"],
                    "meeting-room": ["dry", "clear", "balanced"],
                    "board-room": ["dry", "balanced", "intimate"],
                    
                    # Matériaux
                    "wood": ["warm", "balanced", "live"],
                    "concrete": ["bright", "reverberant", "harsh"],
                    "stone": ["reverberant", "bright", "live"],
                    "marble": ["reverberant", "bright", "harsh"],
                    "glass": ["bright", "harsh", "live"],
                    "metal": ["bright", "harsh", "resonant"],
                    "fabric": ["dead", "dry", "absorptive"],
                    "carpet": ["dead", "dry", "absorptive"],
                    "brick": ["warm", "diffuse", "balanced"],
                    "plaster": ["balanced", "diffuse", "live"],
                    "tile": ["bright", "reverberant", "harsh"],
                    
                    # Architecture
                    "dome": ["focused", "resonant", "reverberant"],
                    "vault": ["reverberant", "diffuse", "resonant"],
                    "column": ["diffuse", "live", "spacious"],
                    "pillar": ["diffuse", "live", "spacious"],
                    "arch": ["diffuse", "resonant", "spacious"],
                    "spire": ["focused", "resonant", "bright"],
                    "steeple": ["focused", "resonant", "bright"],
                    "alcove": ["focused", "intimate", "resonant"],
                    "corridor": ["focused", "reverberant", "bright"],
                    "nave": ["reverberant", "spacious", "diffuse"],
                    
                    # Caractéristiques spatiales
                    "large": ["reverberant", "spacious", "diffuse"],
                    "small": ["intimate", "dry", "focused"],
                    "high": ["reverberant", "bright", "spacious"],
                    "low": ["intimate", "warm", "focused"],
                    "wide": ["spacious", "diffuse", "reverberant"],
                    "narrow": ["focused", "intimate", "resonant"],
                    "open": ["diffuse", "spacious", "clear"],
                    "enclosed": ["intimate", "focused", "resonant"],
                    "spacious": ["diffuse", "reverberant", "clear"],
                    "cramped": ["intimate", "focused", "dead"],
                    "tall": ["reverberant", "bright", "diffuse"],
                    "short": ["intimate", "warm", "focused"]
                }
                
                # Dictionnaire pour compter les propriétés mentionnées
                property_counts = {prop: 0 for prop in core_acoustic_properties}
                
                # Analyser les caractéristiques par catégorie
                for p_type in selected_place_types:
                    if p_type in acoustic_mapping:
                        for prop in acoustic_mapping[p_type]:
                            if prop in property_counts:
                                property_counts[prop] += 3  # Le type de lieu a une forte influence
                
                for mat in selected_materials:
                    if mat in acoustic_mapping:
                        for prop in acoustic_mapping[mat]:
                            if prop in property_counts:
                                property_counts[prop] += 2  # Les matériaux ont une influence modérée
                
                for arch in folder_keywords_by_category['architecture']:
                    if arch in acoustic_mapping:
                        for prop in acoustic_mapping[arch]:
                            if prop in property_counts:
                                property_counts[prop] += 2  # L'architecture a une influence modérée
                
                for spat in folder_keywords_by_category['spatial']:
                    if spat in acoustic_mapping:
                        for prop in acoustic_mapping[spat]:
                            if prop in property_counts:
                                property_counts[prop] += 1  # Les caractéristiques spatiales ont une influence plus légère
                
                # Sélectionner les propriétés les plus pertinentes (max 3)
                sorted_properties = sorted(property_counts.items(), key=lambda x: x[1], reverse=True)
                inferred_properties = [prop for prop, count in sorted_properties if count > 0][:3]
                
                if inferred_properties:
                    print(f"Propriétés acoustiques inférées: {', '.join(inferred_properties)}")
                    # Ajouter les propriétés inférées
                    folder_keywords_by_category['acoustic_properties'].extend(inferred_properties)
                    all_keywords.extend(inferred_properties)
            
            # 3. Sélectionner les mots-clés pour les autres catégories
            # Pour architecture et acoustic_properties : prendre le plus fréquent
            for category in ["architecture", "acoustic_properties"]:
                # Prendre le mot-clé le plus fréquent de cette catégorie
                category_keywords = [(word, freq) for word, freq in category_counters[category].items() if freq >= 1]
                category_keywords.sort(key=lambda x: x[1], reverse=True)
                
                if category_keywords:
                    selected_keywords.append(category_keywords[0][0])
                elif category == "acoustic_properties" and "acoustic_properties" in folder_keywords_by_category and folder_keywords_by_category["acoustic_properties"]:
                    # Si nous avons des propriétés acoustiques inférées, prendre la première
                    selected_keywords.append(folder_keywords_by_category["acoustic_properties"][0])
            
            # Pour la catégorie spatiale, prendre jusqu'à 3 termes, avec priorité aux descripteurs de forme
            shape_descriptors = [
                "circular", "curved", "rounded", "square", "rectangular", "oval", 
                "triangular", "hexagonal", "octagonal", "spherical", "cylindrical", 
                "conical", "pyramidal", "cubical", "elliptical", "spiral"
            ]
            
            # D'abord les descripteurs de taille (comme "large", "small")
            size_keywords = [(word, freq) for word, freq in category_counters['spatial'].items() 
                            if freq >= 1 and word not in shape_descriptors]
            size_keywords.sort(key=lambda x: x[1], reverse=True)
            
            # Ensuite les descripteurs de forme
            shape_keywords = [(word, freq) for word, freq in category_counters['spatial'].items() 
                             if freq >= 1 and word in shape_descriptors]
            shape_keywords.sort(key=lambda x: x[1], reverse=True)
            
            # Sélectionner au moins un descripteur de taille si disponible
            if size_keywords:
                selected_keywords.append(size_keywords[0][0])
            
            # Ajouter tous les descripteurs de forme (jusqu'à 2)
            for shape, _ in shape_keywords[:2]:
                selected_keywords.append(shape)
            
            # Filtrer les contradictions
            selected_keywords = filter_contradictory_keywords(selected_keywords, Counter(all_keywords))
            
            # Filtrer les synonymes
            selected_keywords = filter_synonyms(selected_keywords, Counter(all_keywords))
            
            # Éliminer les doublons
            selected_keywords = list(dict.fromkeys(selected_keywords))
            
            # Limiter au maximum de mots-clés
            if len(selected_keywords) > max_keywords:
                selected_keywords = selected_keywords[:max_keywords]
            
            # Si nous n'avons pas assez de mots-clés, ajouter "NoLabel" pour compléter
            if len(selected_keywords) < min_keywords:
                needed = min_keywords - len(selected_keywords)
                print(f"Pas assez de mots-clés ({len(selected_keywords)}/{min_keywords}), ajout de {needed} \"NoLabel\"")
                selected_keywords.extend(["NoLabel"] * needed)
            
            print(f"\nDESCRIPTEURS FINAUX: {', '.join(selected_keywords)}")
            
            # Créer la chaîne de mots-clés
            keywords_str = "_".join(selected_keywords)
            
            # Si aucun mot-clé n'a été trouvé, utiliser "NoLabel" comme unique descripteur
            if not keywords_str:
                keywords_str = "NoLabel"
            
            if len(keywords_str) > 240:
                keywords_str = keywords_str[:240]
            
            # Traiter les dossiers audio
            parent_dir = Path(folder).parent
            bformat_dir = os.path.join(parent_dir, "BFormatToStereo")
            stereo_dir = os.path.join(parent_dir, "stereo")
            
            if os.path.exists(bformat_dir):
                bformat_labelled_dir = os.path.join(parent_dir, "BFormatToStereoLabelled")
                try:
                    shutil.copytree(bformat_dir, bformat_labelled_dir)
                except Exception as e:
                    print(f"ERREUR lors de la copie du dossier {bformat_dir} vers {bformat_labelled_dir}: {e}")
                    raise
                
                # Renommer les fichiers audio
                for audio_file in glob.glob(os.path.join(bformat_labelled_dir, "*")):
                    if os.path.isfile(audio_file):
                        audio_ext = os.path.splitext(audio_file)[1]
                        new_audio_path = os.path.join(bformat_labelled_dir, f"{keywords_str}{audio_ext}")
                        
                        counter = 1
                        base_path = new_audio_path
                        while os.path.exists(new_audio_path) and new_audio_path != audio_file:
                            name_part = os.path.splitext(base_path)[0]
                            new_audio_path = f"{name_part}_{counter}{audio_ext}"
                            counter += 1
                        
                        if new_audio_path != audio_file:
                            try:
                                os.rename(audio_file, new_audio_path)
                            except Exception as e:
                                print(f"ERREUR lors du renommage de {audio_file}: {e}")
                                raise
            
            if os.path.exists(stereo_dir):
                stereo_labelled_dir = os.path.join(parent_dir, "stereoLabelled")
                try:
                    shutil.copytree(stereo_dir, stereo_labelled_dir)
                except Exception as e:
                    print(f"ERREUR lors de la copie du dossier {stereo_dir} vers {stereo_labelled_dir}: {e}")
                    raise
                
                # Renommer les fichiers audio
                for audio_file in glob.glob(os.path.join(stereo_labelled_dir, "*")):
                    if os.path.isfile(audio_file):
                        audio_ext = os.path.splitext(audio_file)[1]
                        new_audio_path = os.path.join(stereo_labelled_dir, f"{keywords_str}{audio_ext}")
                        
                        counter = 1
                        base_path = new_audio_path
                        while os.path.exists(new_audio_path) and new_audio_path != audio_file:
                            name_part = os.path.splitext(base_path)[0]
                            new_audio_path = f"{name_part}_{counter}{audio_ext}"
                            counter += 1
                        
                        if new_audio_path != audio_file:
                            try:
                                os.rename(audio_file, new_audio_path)
                            except Exception as e:
                                print(f"ERREUR lors du renommage de {audio_file}: {e}")
                                raise
        
        except Exception as e:
            print(f"\nERREUR CRITIQUE lors du traitement du dossier {folder}:")
            print(f"Détail: {e}")
            print("\nTraitement interrompu à cause d'une erreur.")
            sys.exit(1)

if __name__ == "__main__":
    import argparse
    
    parser = argparse.ArgumentParser(description="Analyse des images par dossier pour les IRs acoustiques")
    parser.add_argument("--root_dir", type=str, required=True, help="Répertoire racine à partir duquel chercher des images")
    parser.add_argument("--min_keywords", type=int, default=5, help="Nombre minimal de mots-clés à extraire par dossier")
    parser.add_argument("--max_keywords", type=int, default=15, help="Nombre maximal de mots-clés à extraire par dossier")
    
    args = parser.parse_args()
    
    try:
        process_images_by_folder(args.root_dir, args.min_keywords, args.max_keywords)
        print("\nTraitement terminé avec succès!")
    except Exception as e:
        print(f"\nERREUR FATALE pendant l'exécution:")
        print(f"Détail: {e}")
        sys.exit(1)