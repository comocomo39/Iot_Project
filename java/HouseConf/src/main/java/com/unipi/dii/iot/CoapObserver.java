package com.unipi.dii.iot;

import org.eclipse.californium.core.CoapClient;
import org.eclipse.californium.core.CoapHandler;
import org.eclipse.californium.core.CoapObserveRelation;
import org.eclipse.californium.core.CoapResponse;
import org.json.simple.JSONArray;
import org.json.simple.JSONObject;
import org.json.simple.parser.JSONParser;

/**
 * CoapObserver - Gestisce l'osservazione di una risorsa CoAP.
 */
public class CoapObserver implements Runnable {

    private CoapClient client;
    private CoapObserveRelation relation;
    private String name;
    private String ipv6;
    private IPv6DatabaseManager dbManager = new IPv6DatabaseManager(); // creato una sola volta

    public CoapObserver(String ipv6, String name) {
        String uri = "coap://[" + ipv6 + "]:5683/monitoring";
        client = new CoapClient(uri);
        this.ipv6 = ipv6;
        this.name = name;
    }

    public void startObserving() {
        relation = client.observe(new CoapHandler() {
            @Override
            public void onLoad(CoapResponse response) 
            {
                IPv6DatabaseManager dbManger= new IPv6DatabaseManager();
                String content = response.getResponseText();
                System.out.println("Notification: " + content);
                JSONObject json= null;
                try{
                   JSONParser parser = new JSONParser();
                   if(name.equals("env_sample")){
                    json = (JSONObject) parser.parse(content);
                    Long timeid=(Long) json.get("t");
                    JSONArray ssArray =(JSONArray) json.get("ss");
                    dbManger.insertSensorENV("env_sample", ipv6, ssArray);
                   }
                   else if(name.equals("air_sample")) {
                    json = (JSONObject) parser.parse(content);
                    if (json.containsKey("ss")) {
                        Long timeid = (Long) json.get("t");
                        JSONArray ssArray = (JSONArray) json.get("ss");
                        dbManger.insertSensorAIR("air_sample", ipv6,ssArray, timeid);
                    } else {
                        System.out.println("Il JSON non contiene 'ss'");
                    }
                }
                } catch (Exception e) {
                    System.err.println("Errore durante il parsing o l'inserimento: " + e.getMessage());
                }
            }


            @Override
            public void onError() {
                System.err.println("Failed to receive notification from " + ipv6);
            }
        });
    }

    public void stopObserving() {
        if (relation != null) {
            relation.proactiveCancel();
        }
        if (client != null) {
            client.shutdown();
        }
    }

    @Override
    public void run() {
        startObserving();
    }
}
