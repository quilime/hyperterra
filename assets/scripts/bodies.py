# -*- coding: utf-8 -*-
#Allows for use of °

'''
to install pyephem on OSX
    sudo ARCHFLAGS=-Wno-error=unused-command-line-argument-hard-error-in-future pip install pyephem

A new version of ephem.test. cleaned for project flashlight
https://pypi.python.org/pypi/pyephem/
http://rhodesmill.org/pyephem/
'PyEphem provides scientific-grade astronomical computations for Python'

--Extracts clean data regarding the precise apparent location of any cosmic body--

Author: Rob Godshaw
Modified: Gabriel Dunne

Inputs: Time, Lattitude, Longitude, Altitude(above sea level)

Outputs: Apparent Azimuth, Altitude(above horizon) and %illumination of the moon

'''
import ephem
import math

# http://www.daftlogic.com/sandbox-google-maps-find-altitude.htm
# PlaceName = (latitude°,longitude°, elevation above sea level in meters)
Pier9           = (37.7999667,-122.39800439999999, 9.031)
Emeryville      = (0,37.831316, -122.28524729999998, 24.407)
TreasureIsland  = (37.8235515, -122.37064800000002, 11.532)
currentPosition = Pier9
currPosStr      = "Pier 9, San Francisco"

# set position
position = ephem.Observer()
position.lat, position.lon, position.elevation = currentPosition
position.date = ephem.now()

# convert heading (degrees) into cardinal direction
def headingToCardinal(heading):
    fullCirlce=360.0
    cardinalQty =16
    wedgeSize = fullCirlce /cardinalQty
    offset = wedgeSize / 2.0
    cardinalIndex = int(((heading + offset) % fullCirlce) / wedgeSize)
    cardinalList = \
    ['N','NNE','NE','ENE','E','ESE','SE','SSE',\
     'S','SSW','SW', 'WSW','W','WNW','NW','NNW']
    return cardinalList[cardinalIndex]

# convert pitch (in degrees) to vernacular string
def pitchToVernacular(pitch):
    fullCirlce = 360.0
    halfCircle = fullCirlce / 2
    quarterCirle = halfCircle / 2
    cardinalQty = 16
    wedgeSize = fullCirlce /cardinalQty
    offset = wedgeSize / 2.0
    angleIndex = int(((pitch + quarterCirle + offset) % fullCirlce) / wedgeSize)
    angleList = [
        'Directly underfoot', # -90
        'Not quite directly underfoot',
        'Way below horizon', # -45°
        'below horizon',
        'At horizon', # ------0°------
        'Above horizon',
        'High', #45°
        'Not quite Directly overhead',
        'Directly overhead', #90
    ]
    return angleList[angleIndex]

# get altitude and azimuth of body
def printPosition(body):
    body.compute(position)
    altitude = math.degrees(float(body.alt))
    azimuth  = math.degrees(float(body.az))

    print "\taltitude: %s"%altitude
    print "\tazimuth: %s"%azimuth

    print '\tazimuth is %s (%.1f° clockwise from magnetic north)'%(headingToCardinal(azimuth),azimuth)
    print '\televation is %s (%.1f° %s horizon)' %\
    (pitchToVernacular(altitude),altitude,\
        ('above' if altitude > 0 else 'below'))    


if __name__ == '__main__':
    print "UTC " + str(ephem.now())
    print 'Observer position: ' + currPosStr
    print "lng, lat, alt"
    print currentPosition

    print "\nsun:"
    printPosition(ephem.Moon())

    print "\nmoon:"
    printPosition(ephem.Sun())
    #printCurrentSolarPosition()





# A samplng of the bodies we can track is below
Mercury = ephem.Mercury()
Mercury.compute(position)

Venus = ephem.Venus()
Venus.compute(position)

Mars = ephem.Mars()
Mars.compute(position)

Jupiter = ephem.Jupiter()
Jupiter.compute(position)

Saturn = ephem.Saturn()
Saturn.compute(position)

Uranus = ephem.Uranus()
Uranus.compute(position)

Pluto = ephem.Pluto()
Pluto.compute(position)

Phobos = ephem.Phobos()
Phobos.compute(position)

Deimos = ephem.Deimos()
Deimos.compute(position)

Io = ephem.Io()
Io.compute(position)

Europa = ephem.Europa()
Europa.compute(position)

Ganymede = ephem.Ganymede()
Ganymede.compute(position)

Callisto = ephem.Callisto()
Callisto.compute(position)

Mimas = ephem.Mimas()
Mimas.compute(position)

Enceladus = ephem.Enceladus()
Enceladus.compute(position)

Tethys = ephem.Tethys()
Tethys.compute(position)

Dione = ephem.Dione()
Dione.compute(position)

Rhea = ephem.Rhea()
Rhea.compute(position)

Titan = ephem.Titan()
Titan.compute(position)

Hyperion = ephem.Hyperion()
Hyperion.compute(position)

Iapetus = ephem.Iapetus()
Iapetus.compute(position)

Ariel = ephem.Ariel()
Ariel.compute(position)

Umbriel = ephem.Umbriel()
Umbriel.compute(position)

Titania = ephem.Titania()
Titania.compute(position)

Oberon = ephem.Oberon()
Oberon.compute(position)

Miranda = ephem.Miranda()
Miranda.compute(position)

Neptune = ephem.Neptune()
Neptune.compute(position)




#Some of the brightest stars



Sirius = ephem.star("Sirius")
Sirius.compute(position)

Canopus = ephem.star("Canopus")
Canopus.compute(position)

Arcturus = ephem.star("Arcturus")
Arcturus.compute(position)

Vega = ephem.star("Vega")
Vega.compute(position)

Capella = ephem.star("Capella")
Capella.compute(position)

Rigel = ephem.star("Rigel")
Rigel.compute(position)

Procyon = ephem.star("Procyon")
Procyon.compute(position)

Achernar = ephem.star("Achernar")
Achernar.compute(position)

Betelgeuse = ephem.star("Betelgeuse")
Betelgeuse.compute(position)

Altair = ephem.star("Altair")
Altair.compute(position)

Aldebaran = ephem.star("Aldebaran")
Aldebaran.compute(position)

Antares = ephem.star("Antares")
Antares.compute(position)

Spica = ephem.star("Spica")
Spica.compute(position)

Pollux = ephem.star("Pollux")
Pollux.compute(position)

Fomalhaut = ephem.star("Fomalhaut")
Fomalhaut.compute(position)

Deneb = ephem.star("Deneb")
Deneb.compute(position)

Regulus = ephem.star("Regulus")
Regulus.compute(position)

Castor = ephem.star("Castor")
Castor.compute(position)

Shaula = ephem.star("Shaula")
Shaula.compute(position)

Polaris = ephem.star("Polaris")
Polaris.compute(position)
