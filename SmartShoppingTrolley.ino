#include "secrets.h"
#include <SPI.h>
#include <MFRC522.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <WiFi.h>
#include <WebServer.h>
#include "HX711.h"

#define SS_PIN 5
#define RST_PIN 4
#define BUZZER 25

#define DT 26
#define SCK 27

MFRC522 rfid(SS_PIN, RST_PIN);
LiquidCrystal_I2C lcd(0x27,16,2);
WebServer server(80);
HX711 scale;

const char* ssid = WIFI_SSID;
const char* password = WIFI_PASSWORD;

int total = 0;

bool milk=false;
bool bread=false;
bool biscuit=false;


// weight timer
unsigned long lastWeightRead = 0;
const int weightInterval = 2000;


// -------- BUZZER --------
void addBeep()
{
digitalWrite(BUZZER,HIGH);
delay(500);
digitalWrite(BUZZER,LOW);
}

void removeBeep()
{
for(int i=0;i<2;i++)
{
digitalWrite(BUZZER,HIGH);
delay(200);
digitalWrite(BUZZER,LOW);
delay(200);
}
}


/////////////////////////////////////////////////
// CART PAGE
/////////////////////////////////////////////////

void handleRoot()
{

String items = "";

if(milk)
items += "<div class='flex justify-between'><span>Milk</span><span>&#8377;40</span></div>";

if(bread)
items += "<div class='flex justify-between'><span>Bread</span><span>&#8377;30</span></div>";

if(biscuit)
items += "<div class='flex justify-between'><span>Biscuit</span><span>&#8377;20</span></div>";

if(items=="")
items="<p>No items in cart</p>";

String page = R"====(
<!DOCTYPE html>
<html>
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1.0">
<title>Smart Trolley</title>
<script src="https://cdn.tailwindcss.com"></script>
<meta http-equiv="refresh" content="3">
</head>

<body class="bg-slate-100">

<div class="max-w-md mx-auto p-6">

<h1 class="text-2xl font-bold text-center mb-4">
SMART TROLLEY
</h1>

<div class="bg-white rounded-xl shadow p-4 mb-4">
)====" + items + R"====(
</div>

<div class="bg-white rounded-xl shadow p-4 mb-4">
<p class="text-lg font-semibold">
Total : &#8377;)====" + String(total) + R"====(
</p>
</div>

<a href="/checkout"
class="block w-full py-3 text-center bg-emerald-600 text-white rounded-lg">
Checkout
</a>

</div>

</body>
</html>
)====";

server.send(200,"text/html",page);
}



/////////////////////////////////////////////////
// CHECKOUT PAGE
/////////////////////////////////////////////////

void handleCheckout()
{

String page = R"====(
<!DOCTYPE html>
<html>
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1.0">
<title>Checkout</title>
<script src="https://cdn.tailwindcss.com"></script>
<script src="https://checkout.razorpay.com/v1/checkout.js"></script>
</head>

<body class="bg-slate-100">

<div class="max-w-md mx-auto p-6">

<h1 class="text-2xl font-bold mb-4">
Checkout
</h1>

<div class="bg-white rounded-xl p-4 mb-4">
<p>Total : &#8377;)====" + String(total) + R"====(</p>
</div>

<button onclick="payNow()"
class="w-full py-3 bg-emerald-600 text-white rounded-lg">
Pay with UPI
</button>

</div>

<script>

function payNow()
{

var options = {

key: "rzp_test_SNyw10hscxu4en",//don't use or share only for demo test

amount: )====" + String(total*100) + R"====(,

currency: "INR",

name: "Smart Trolley",

description: "Cart Payment",

handler: function (response)
{
alert("Payment Successful");
window.location.href="/";
},

theme:{color:"#059669"}

};

var rzp = new Razorpay(options);
rzp.open();

}

</script>

</body>
</html>
)====";

server.send(200,"text/html",page);
}



/////////////////////////////////////////////////
// SETUP
/////////////////////////////////////////////////

void setup()
{

Serial.begin(115200);

SPI.begin();
rfid.PCD_Init();

Wire.begin(21,17);

lcd.init();
lcd.backlight();

pinMode(BUZZER,OUTPUT);

scale.begin(DT,SCK);
scale.set_scale(420);
scale.tare();

lcd.setCursor(0,0);
lcd.print("Smart Trolley");

WiFi.begin(ssid,password);

lcd.setCursor(0,1);
lcd.print("Connecting WiFi");

while(WiFi.status()!=WL_CONNECTED)
{
delay(500);
Serial.print(".");
}

Serial.println(WiFi.localIP());

lcd.clear();
lcd.print("WiFi Connected");

server.on("/",handleRoot);
server.on("/checkout",handleCheckout);

server.begin();

delay(2000);

lcd.clear();
lcd.print("Scan Product");

}



/////////////////////////////////////////////////
// LOOP
/////////////////////////////////////////////////

void loop()
{

server.handleClient();


// weight read every 2 seconds
if(millis() - lastWeightRead > weightInterval)
{
lastWeightRead = millis();

float weight = scale.get_units(5);

Serial.print("Weight: ");
Serial.println(weight);
}



if (!rfid.PICC_IsNewCardPresent()) return;
if (!rfid.PICC_ReadCardSerial()) return;

String uid="";

for(byte i=0;i<rfid.uid.size;i++)
uid += String(rfid.uid.uidByte[i],HEX);

uid.toLowerCase();
Serial.println(uid);
lcd.clear();


// MILK
if(uid=="52986f5")
{
if(!milk)
{
milk=true;
total+=40;
lcd.print("Milk Added");
addBeep();
}
else
{
milk=false;
total-=40;
lcd.print("Milk Removed");
removeBeep();
}
}


// BREAD
else if(uid=="de65a94")
{
if(!bread)
{
bread=true;
total+=30;
lcd.print("Bread Added");
addBeep();
}
else
{
bread=false;
total-=30;
lcd.print("Bread Removed");
removeBeep();
}
}


// BISCUIT
else if(uid=="abb35d1")
{
if(!biscuit)
{
biscuit=true;
total+=20;
lcd.print("Biscuit Added");
addBeep();
}
else
{
biscuit=false;
total-=20;
lcd.print("Biscuit Removed");
removeBeep();
}
}

else
{

lcd.print("Unknown Item");

digitalWrite(BUZZER,HIGH);
delay(2000);
digitalWrite(BUZZER,LOW);

}
float weight = scale.get_units(5);
lcd.setCursor(0,1);
lcd.print("Weight:");
lcd.print(weight);
delay(1000);
lcd.setCursor(0,1);
lcd.print("Total Rs:");
lcd.print(total);


delay(2000);

lcd.clear();
lcd.print("Scan Product");

}