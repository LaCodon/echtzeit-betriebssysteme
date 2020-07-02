# Gießsteuerung

Nutzt https://www.streamlit.io/ zur Oberflächengestaltung.

Die Konfiguration wird dann in der Datei `calibration.csv` gespeichert. Der Pfad zur Datei
kann über das erste Argument gesteuert werden. Die Datei `test-hydro.csv` enthält Messdaten
des Feuchtesensors zum Anzeigen in der Oberfläche.

## Setup

Python venv erzeugen: `python -m venv venv`

Venv aktivieren: `source ./venv/Scripts/activate`

Abhängigkeiten installieren: `pip install -r requirements.txt`

## Ausführen

Mit aktiviertem venv: `streamlit run main.py -- /pfad/zum/datei/verzeichnis/`