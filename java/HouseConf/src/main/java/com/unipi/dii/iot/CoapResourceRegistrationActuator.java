package com.unipi.dii.iot;

import java.net.InetAddress;
import java.util.List;

import org.eclipse.californium.core.CoapResource;
import org.eclipse.californium.core.coap.CoAP;
import org.eclipse.californium.core.coap.Response;
import org.eclipse.californium.core.server.resources.CoapExchange;
import org.json.simple.JSONObject;

import com.unipi.dii.iot.IPv6DatabaseManager.PairNameIp;

class CoapResourceRegistrationActuator extends CoapResource {

    IPv6DatabaseManager db = new IPv6DatabaseManager();

    public CoapResourceRegistrationActuator(String name) {
        super(name);
    }

    @Override
    public void handleGET(CoapExchange exchange) {
        exchange.respond("hello world");
    }

    @Override
    public void handlePOST(CoapExchange exchange) {
        // Controlla che tutti i sensori siano registrati
        if (!db.checkAllSensorsRegistered()) {
            System.out.println("NOT ALL SENSORS REGISTERED YET!");
            exchange.respond(new Response(CoAP.ResponseCode.BAD_REQUEST));
            return;
        }

        System.out.println("POST ACTUATOR received");

        String payloadString = exchange.getRequestText();
        String ipAddress = exchange.getSourceAddress().getHostAddress();
        System.out.println("Payload received: " + payloadString + " \nlength: " + payloadString.length());
        System.out.println("IP address: " + ipAddress + "\n");

        String actuator = payloadString;

        if (actuator != null && ipAddress != null) {
            try {
                // Controlla se l'attuatore è già registrato
                List<PairNameIp> actuatorIPs = db.getIPs("actuator");
                for (PairNameIp pair : actuatorIPs) {
                    if (pair.ip.equals(ipAddress)) {
                        System.out.println("Actuator IP already registered");
                        exchange.respond(new Response(CoAP.ResponseCode.BAD_REQUEST));
                        return;
                    }
                }

                System.out.println("Inserting ACTUATOR IP in db: " + ipAddress);
                db.insertIPv6Address(ipAddress, "actuator", actuator);
                System.out.println("Actuator IP REGISTERED!");

                // Costruisci il JSON di risposta con l'indirizzo del sensore associato
                List<PairNameIp> sensorIPs = db.getIPs("sensor");
                JSONObject responseJson = new JSONObject();

                String ipv6ToSend = null;

                // Decidi quale sensore associare in base al nome dell'attuatore
                if (actuator.toLowerCase().contains("actuator")) {
                    // cerca "air_sample"
                    for (PairNameIp ip : sensorIPs) {
                        if (ip.name.toLowerCase().contains("air_sample")) {
                            ipv6ToSend = ip.ip;
                            break;
                        }
                    }
                } else if (actuator.toLowerCase().contains("ventilation")) {
                    // cerca "env_sample"
                    for (PairNameIp ip : sensorIPs) {
                        if (ip.name.toLowerCase().contains("env_sample")) {
                            ipv6ToSend = ip.ip;
                            break;
                        }
                    }
                }

                if (ipv6ToSend != null) {
                    responseJson.put("e", ipv6ToSend);
                    System.out.println("✔ IP associato all’attuatore " + actuator + ": " + ipv6ToSend);
                } else {
                    System.out.println("⚠ Nessun sensore corrispondente trovato per " + actuator);
                }

                // Invia risposta JSON all’attuatore
                Response response = new Response(CoAP.ResponseCode.CREATED);
                response.setPayload(responseJson.toJSONString());
                exchange.respond(response);

            } catch (Exception e) {
                System.err.println("Error inserting actuator IP or preparing response: " + e.getMessage());
                exchange.respond(new Response(CoAP.ResponseCode.INTERNAL_SERVER_ERROR));
            }
        } else {
            System.err.println("Missing actuator name or IP address");
            exchange.respond(new Response(CoAP.ResponseCode.BAD_REQUEST));
        }
    }
}
