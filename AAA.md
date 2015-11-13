# Radius #

http://en.wikipedia.org/wiki/List_of_RADIUS_Servers


SQL HOWTO
From FreeRADIUS Wiki
http://wiki.freeradius.org/SQL_HOWTO


HOWTO Chillispot with FreeRadius and MySQLhttp://gentoo-wiki.com/HOWTO_Chillispot_with_FreeRadius_and_MySQL

We choose FreeRADIUS.

## Management ##

we choose the daloRADIUS.

http://infodotnet.blogspot.com/2008/03/free-software-for-freeradius-user.html
dolaRADIUS
daloRADIUS is an advanced RADIUS web management application aimed at managing hotspots and general-purpose ISP deployments. It features user management, graphical reporting, accounting, a billing engine and integrates with GoogleMaps for geo-locating.
daloRADIUS is written in PHP and JavaScript and utilizes a MySQL database. It is based on a FreeRADIUS deployment with MySQL serving as the backend. Among other features it implements ACLs, GoogleMaps integration for locating hotspots/access points visually and many more features.
http://daloradius.wiki.sourceforge.net/

Dialup Admin
The FreeRADIUS server comes including a powerfull web interface written in PHP to administer radius users, called dialupadmin. Dialup Admin supports users either in SQL (MySQL or PostgreSQL are supported) or in LDAP.


Apart from the web pages, it also includes a number of scripts to make the administrator's life a lot easier.

Mikrotik Userman
User manager is a management system that can be used for HotSpot user, PPP (PPtP/PPPoE) users, DHCP users, Wireless users and RouterOS users.


Try mikrotik user manager online demo here with user and password is demo.

FreeRadius Admin Pacakage
A snazzy front-end admin kit for FreeRADIUS that allows for the addition of radius users,user groups, and group attribute templating.


Access controls allow for admin users and 'agents' that can act autonomously. Also admins FreeRADIUS radius attributes.

phpRADmin
phpRADmin is a tool written in PHP intended to handle the administration and provisioning of FreeRADIUS over the Web with MySQL as backend. This interface allows the administrator to config, find, create, edit, delete, and test users on an SQL (MySQL) database, create SQL groups, gather accounting information for a user.


phpRADmin show pies and graphs of your network status, usage audits, administration profiles with 3 levels. phpRADmin is Chillispot and WISP ready.



# Hostapd #

http://hostap.epitest.fi/hostapd/

readme. Introduce how to configure the 802.1x and radius
http://hostap.epitest.fi/gitweb/gitweb.cgi?p=hostap.git;a=blob_plain;f=hostapd/README

# Captive Portal & Radius #

We choose the CoovaChilli.

CoovaChillihttp://coova.org/wiki/index.php/CoovaChilli (Based chillispot)

chillispothttp://www.chillispot.info/index.html

nocatsplashhttp://nocat.net/
nodogsplashhttp://kokoro.ucsd.edu/nodogsplash/

m0n0wall http://m0n0.ch/wall/

## CoovaChilli ##

# WPA/802.1x/EAP #
WPA = 'WiFi protected access' acronym developed by wifi
committee. It implies wireless access protected with
advanced Wireless security features.

802.1x = IEEE standard which defines port based Access control
Mechanism.

EAP = Extensible Authentication protocol, It is a framework
used in wired as well as wireless infrasture that
uses authentication server say RADIUS, to
authenticate users.

Lately, I found EAP,802.1x and WPA terms has been used interchangeably in supposedly "technical" magazine and
I think it is not right.

802.1x is a IEEE standard that defines port based access control, It proposes EAP as one of the authentication method as a method of Advanced authentication using TLS (Transport Layer security)and TTLS(Tunneled Transport Layered Security) , PEAP, LEAP etc. authentication protocols.

WPA is not necessarily EAP, a preshared Key (modelled after classic 10 hex digit WEP) can also be a protection mechanism and thus infrastrutures without EAP like authetication mechanism can be still termed as "with WPA".

802.1x is not necessarily has to be with EAP, It can be with say PPP as well.

Port based Access Control using EAP as per 802.1x standard means:
An access control device (sitting on the edge of the ISP network servicing subscriber/end-user)/Authenticator blocks
L3 connectivity for a particular L2 address untill its UserID+password, userID+client-certificate etc. based autheticity is verified using Authetication Sever.

Host+AP+Radius server combo would work as below:
EAP messages (Access-request, Access-challenge) are encapsulated as L2 payloads (without L3,L4 headers) between Authenticator (say, Access Point) and supplicant(host,laptop).
The responses are relayed as EAP messages wth regular networking heades(UDP-IP packets carrying say Radius headers with EAP-Access Requests) to Authentication Server (say, Radius) of ISP infrastructure.
Responses from Radius/ Authentication server (say Access-Accept)to AP/Autheticator are relayed again as EAP messages as L2-payload from AP/Authenticator to Host/Supplicant.

Supplicant 

&lt;---EAP-L2---&gt;

 Authenticator 

&lt;---EAP-Radius-UDP-IP-L2------&gt;

 Authtentication Server