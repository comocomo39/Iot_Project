package com.unipi.dii.iot;

import java.net.InetAddress;
import java.util.List;

import org.eclipse.californium.core.CoapResource;
import org.eclipse.californium.core.coap.CoAP;
import org.eclipse.californium.core.coap.Response;
import org.eclipse.californium.core.server.resources.CoapExchange;
import org.json.simple.JSONArray;
import org.json.simple.JSONObject;
import org.json.simple.parser.JSONParser;

import com.unipi.dii.iot.IPv6DatabaseManager.PairNameIp;

class CoapResourceRegistrationSensor extends CoapResource {

    IPv6DatabaseManager db = new IPv6DatabaseManager();

    public CoapResourceRegistrationSensor(String name) {
        super(name);
         setObservable(true);
    }

    @Override
    public void handleGET(CoapExchange exchange) {
        exchange.respond("hello world");
    }
    
    @Override
    public void handlePOST(CoapExchange exchange) {
        System.out.println("POST sensor received");
        String payloadString = exchange.getRequestText();
        System.out.println("Payload received: " + payloadString + " \nlength: " + payloadString.length());
        String ipAddress=exchange.getSourceAddress().getHostAddress();        
        JSONObject json = null;
        try {
        JSONParser parser = new JSONParser();
        json = (JSONObject) parser.parse(payloadString);
        
        Response response=null;
        String sensor = (String) json.get("s");
        String ipv6 = ipAddress;
        JSONArray sensingType = (JSONArray) json.get("ss");
        Long timeSample = (Long) json.get("t");
        if (sensor != null && ipv6 != null && sensingType != null ) {
            InetAddress addr = exchange.getSourceAddress();         
                // Insert the sensor IP in the database
                //checking if ip
                List<PairNameIp> sensorIPs = db.getIPs("sensor");
                for (PairNameIp pair : sensorIPs) {
                    if (pair.ip.equals(ipv6)) {
                        System.out.println("Sensor IP already registered");
                        response = new Response(CoAP.ResponseCode.BAD_REQUEST);
                        exchange.respond(response);
                        return;
                    }
                }
                System.out.println("Inserting sensor IP in the "+ sensor);
                db.insertIPv6Address(ipAddress.toLowerCase(), "sensor", sensor.toLowerCase());
                //insert sensor in the database
                IPv6DatabaseManager.createTableSensor(sensor.toLowerCase(), ipv6, sensingType);
                response = new Response(CoAP.ResponseCode.CREATED);
                exchange.respond(response);
                System.out.println("success\n");
                // Start the CoAP observer client
                
                System.out.println("Starting observer client for " );  
                final CoapObserver observerClient = new CoapObserver(ipv6,sensor);
                Thread observertThread=new Thread(observerClient);
                observertThread.start();
                
               
        } else {
            System.err.println("Missing required JSON keys");
            response = new Response(CoAP.ResponseCode.BAD_REQUEST);
        }
        }catch (org.json.simple.parser.ParseException err) {
            System.err.println("JSON parsing error: " + err.getMessage());
            exchange.respond(CoAP.ResponseCode.BAD_REQUEST, "Invalid JSON format");
        }            
        catch (Exception e) {
            System.err.println("Error inserting sensor IP in the database: " + e.getMessage());
            exchange.respond(CoAP.ResponseCode.INTERNAL_SERVER_ERROR, "generic error");
        }
    }
}
