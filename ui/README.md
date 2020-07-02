# Gießsteuerung

Nutzt https://www.streamlit.io/ zur Oberflächengestaltung.

Die Konfiguration wird dann in der Datei `calibration.csv` gespeichert. Der Pfad zur Datei
kann über das erste Argument gesteuert werden. Die Datei `test-hydro.csv` enthält Messdaten
des Feuchtesensors zum Anzeigen in der Oberfläche.

## Setup

Python 3.7 wird benötigt!

Python venv erzeugen: `python -m venv venv`

Venv aktivieren: `source ./venv/bin/activate`

Build requirements installieren: `sudo apt install libjpeg-dev python3.7-dev libatlas-base-dev`

Wheel installieren: `pip install wheel`

Abhängigkeiten installieren: `pip install -r requirements.txt`

## Ausführen

Mit aktiviertem venv: `streamlit run main.py -- /pfad/zum/datei/verzeichnis/`
