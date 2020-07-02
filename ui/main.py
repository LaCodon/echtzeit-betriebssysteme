import streamlit as st
import pandas as pd
import csv
import sys
import getopt

filesDirectory = "./"

if len(sys.argv) > 1:
    filesDirectory = sys.argv[1]

"""
# Gießsteuerung

Das ist das Frontend für die Gießsteuerung im Modul Echtzheitbetriebssysteme von
Fabian Maier und Tim Schmidt.
"""

calibData = pd.read_csv(filesDirectory + "calibration.csv")
loadedArid = calibData["values"][0]
loadedHumid = calibData["values"][1]
loadedMilliliter = calibData["values"][2]

hydro_data = pd.read_csv(filesDirectory + "test-hydro.csv", sep=";")
chart_data = pd.DataFrame(hydro_data)

"## Kalibrierung"

arid = st.number_input(label="Oberer Sensorwert für 'Erde trocken'",
                       value=loadedArid,
                       step=100)
humid = humid = st.number_input(label="Unterer Sensorwert für 'Erde feucht'",
                                value=loadedHumid,
                                step=100)

chart_data["trocken"] = arid
chart_data["feucht"] = humid

"## Monitoring"
st.button("Neu laden")
"Aktuell gemessener Feuchtigkeitswert (je kleiner desto feuchter):"

st.line_chart(chart_data)

"## Automatisierung"

milliliter = st.number_input(label="Zu gießende Menge Wasser in Milliliter, wenn trocken",
                             value=loadedMilliliter,
                             step=100)

with open(filesDirectory + "calibration.csv", "w") as file:
    file.write("values\n")
    file.write(str(arid) + "\n")
    file.write(str(humid) + "\n")
    file.write(str(milliliter))
