#!/usr/bin/env python3
"""
Script pour uploader le fichier .env vers le système de fichiers SPIFFS de l'ESP32
Usage: python3 scripts/upload_env.py [--port /dev/ttyUSB0] [--env-file .env]
"""

import argparse
import os
import sys
import subprocess
import tempfile

def main():
    parser = argparse.ArgumentParser(description='Upload .env file to ESP32 SPIFFS')
    parser.add_argument('--port', '-p', help='Serial port (auto-detect if not specified)')
    parser.add_argument('--env-file', '-e', default='.env', help='Path to .env file (default: .env)')
    parser.add_argument('--baudrate', '-b', default='115200', help='Baudrate (default: 115200)')
    
    args = parser.parse_args()
    
    # Vérifier que le fichier .env existe
    if not os.path.exists(args.env_file):
        print(f"❌ Fichier {args.env_file} non trouvé!")
        print("💡 Copiez env.example vers .env et modifiez les valeurs")
        return 1
    
    print(f"📁 Lecture du fichier {args.env_file}...")
    
    # Lire le contenu du fichier .env
    try:
        with open(args.env_file, 'r', encoding='utf-8') as f:
            env_content = f.read()
    except Exception as e:
        print(f"❌ Erreur lors de la lecture de {args.env_file}: {e}")
        return 1
    
    print(f"📄 Contenu du fichier ({len(env_content)} caractères):")
    print("-" * 40)
    print(env_content)
    print("-" * 40)
    
    # Créer un fichier temporaire pour SPIFFS
    with tempfile.NamedTemporaryFile(mode='w', suffix='.txt', delete=False) as temp_file:
        temp_file.write(env_content)
        temp_path = temp_file.name
    
    try:
        # Construire la commande pio
        cmd = ['pio', 'run', '--target', 'uploadfs']
        if args.port:
            cmd.extend(['--upload-port', args.port])
        
        print(f"🔄 Upload vers SPIFFS...")
        print(f"💻 Commande: {' '.join(cmd)}")
        
        # Note: Cette approche nécessite que le fichier soit dans le dossier data/
        data_dir = 'data'
        if not os.path.exists(data_dir):
            os.makedirs(data_dir)
        
        env_dest = os.path.join(data_dir, '.env')
        
        # Copier le fichier .env vers data/.env
        with open(env_dest, 'w', encoding='utf-8') as f:
            f.write(env_content)
        
        print(f"📋 Fichier copié vers {env_dest}")
        
        # Exécuter l'upload SPIFFS
        result = subprocess.run(cmd, capture_output=True, text=True)
        
        if result.returncode == 0:
            print("✅ Upload SPIFFS réussi!")
            print("🔄 Redémarrez l'ESP32 pour charger la nouvelle configuration")
        else:
            print("❌ Erreur lors de l'upload SPIFFS:")
            print(result.stderr)
            return 1
            
    except subprocess.CalledProcessError as e:
        print(f"❌ Erreur PlatformIO: {e}")
        return 1
    except Exception as e:
        print(f"❌ Erreur: {e}")
        return 1
    finally:
        # Nettoyer le fichier temporaire
        try:
            os.unlink(temp_path)
        except:
            pass
    
    return 0

if __name__ == '__main__':
    sys.exit(main())
