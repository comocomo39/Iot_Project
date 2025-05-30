package com.unipi.remote;

import java.io.IOException;
import java.io.StringReader;
import java.util.List;
import java.util.Scanner;

import javax.json.Json;
import javax.json.JsonException;
import javax.json.JsonObject;
import javax.json.JsonReader;

import org.eclipse.californium.core.CoapClient;
import org.eclipse.californium.core.CoapResponse;
import org.eclipse.californium.core.coap.MediaTypeRegistry;
import org.eclipse.californium.elements.exception.ConnectorException;

import com.unipi.remote.databaseHelper.PairNameIp;



public class RemoteControlApp {

    static databaseHelper db = new databaseHelper();

    public static void main(String[] args) throws ConnectorException, IOException {
        Scanner scanner = new Scanner(System.in);

        while (true) {
            System.out.println("Remote Control Application");
            System.out.println("1. set the temperature threshold for the actuators"); //DONE
            System.out.println("2. Show status of actuators"); //DONE
            System.out.println("3. Get the number of danger events"); //DONE
            System.out.println("4. Turn off a node"); //DONE 
            System.out.println("5. exit"); //DONE
            System.out.print("\n chose and option: ");
        try{
            int choice = scanner.nextInt();
            scanner.nextLine();

            switch (choice) {
                case 1:
                    // set the temperature threshold for the actuators
                    System.out.print("Set the threshold for the ventilation: ");
                    float Ventilationthr = scanner.nextFloat();

                    System.out.print("Set the threshold for the air system: ");
                    float AirSistemthr = scanner.nextFloat();


                    String ipv6 =db.setActuatorTemperatureThreshold("ventilation");
                    String ipv6AirSystem =db.setActuatorTemperatureThreshold("actuator");

                    if(ipv6 == null || ipv6AirSystem == null)
                    {
                        System.out.println("No actuator found: " + ipv6 + " " + ipv6AirSystem);
                        break;
                    }

                    String uri = "coap://[" + ipv6 + "]:5683/threshold";
                    CoapClient client = new CoapClient(uri);
                    String payload = Float.toString(Ventilationthr);
                    CoapResponse response = client.post(payload, MediaTypeRegistry.TEXT_PLAIN);

                    String uriAir = "coap://[" + ipv6AirSystem + "]:5683/threshold";
                    CoapClient client2 = new CoapClient(uriAir);
                    String payload2 = Float.toString(AirSistemthr);
                    CoapResponse response2 = client2.post(payload2, MediaTypeRegistry.TEXT_PLAIN);

                    if (response != null) {
                        System.out.println("Response env_sample: " + response.getResponseText());
                    } else {
                        System.out.println("No response from ventilation.");
                    }
                    if (response2 != null) {
                        System.out.println("Response air system: " + response2.getResponseText());
                    } else {
                        System.out.println("No response from air system.");
                    }
                    break;

                case 2:
                    // show status of actuators
                    Boolean checkDevice0 = false;
                    String nodeName0 = null;
                    String filter = null;

                    while(!checkDevice0){
                        System.out.print("insert the node to check: ");
                        nodeName0 = scanner.nextLine();
                        //checking of the device is a valid one
                        if (nodeName0.equals("actuator") || nodeName0.equals("ventilation")) {
                            checkDevice0 = true;
                            filter="actuator";

                        }else if(nodeName0.equals("air_sample") || nodeName0.equals("env_sample"))
                        {
                            checkDevice0 = true;
                            filter="sensor";
                        }
                        else{
                            System.out.println("Invalid device name");
                        }
                        
                        List<PairNameIp> ips = db.getIPs(filter);   // non restituisce mai null

                        boolean presente = false;
                        for (PairNameIp p : ips) {                  // ciclo classico, niente lambda
                            if (p.name.equalsIgnoreCase(nodeName0)) {
                                presente = true;
                                break;                              // trovato: esci subito
                            }
                        }
                        if (!presente) {
                            System.out.println(nodeName0 + " NON è attivo\n");
                        } else {
                            System.out.println(nodeName0 + " è attivo\n");
                        }

                        
                    }

                    
                    break;


                case 3:
                    // get the number of danger events
                    List<PairNameIp> ips = db.getIPs("sensor");
                    String ip = null;
                    for (PairNameIp pair : ips) {
                        if(pair.name.equals("air_sample"))
                        {
                            ip = pair.ip;
                        }
                    }
                    if(ip==null)
                    {
                        System.out.println("No sensor found\n");
                        break;
                    }
                    
                    CoapClient clientDanger = new CoapClient("coap://["+ ip  + "]:5683/dangerCount");
                    System.out.println("RICHIEDO DANGER A coap://[" +ip  + "]:5683/dangerCount");
                    try {
                        CoapResponse responseDanger = clientDanger.get();
                        if (responseDanger == null || !responseDanger.isSuccess()) {
                          System.out.println("Nessuna risposta dal nodo per /danger");
                        } else {
                          String text = responseDanger.getResponseText().trim();
                          int dangerCount = Integer.parseInt(text);
                          System.out.println("Numero di eventi danger: " + dangerCount);
                        }
                      } catch (ConnectorException | IOException e) {
                        System.out.println("Errore CoAP: " + e.getMessage());
                      } catch (NumberFormatException e) {
                        System.out.println("Risposta non è un intero: " + e.getMessage());
                      }
                      
                    break;             

                case 4:
                    // Turn off a node
                    Boolean checkDevice = false;
                    String nodeName = null;

                    //asking the user for the name of the node to turn off
                    while(!checkDevice){
                        System.out.print("Enter the name of the node to turn off: ");
                        nodeName = scanner.nextLine();
                        //checking of the device is a valid one
                        if (nodeName.equals("actuator") || nodeName.equals("ventilation") || nodeName.equals("air_sample") || nodeName.equals("env_sample")) {
                            checkDevice = true;
                        }
                        else{
                            System.out.println("Invalid device name");
                        }
                    }
                    String filterDb = "sensor";
                    if(nodeName.equals("actuator") || nodeName.equals("ventilation"))
                    {
                        filterDb = "actuator";
                        List<PairNameIp> ipsToContact2 = db.getIPs(filterDb);

                    String ipcont2 = null;

                    for (PairNameIp pair : ipsToContact2) {
                        System.out.println("name e ip: " + pair.name + " " + pair.ip);
                        if(pair.name.equals(nodeName))
                        {
                            ipcont2 = pair.ip;
                        }
                    }
                    CoapClient client4 = new CoapClient("coap://[" + ipcont2 + "]:5683/shutdown");
                    System.out.println("RICHIEDO SHUTDOWN A coap://[" +ipcont2 + "]:5683/shutdown");
                    CoapResponse response4 = client4.get();

                    if (response4 != null) {
                        System.out.println(nodeName + " is shutted\n");
                        //remove from ipv6_addresses the device ip
                        db.removeIp(ipcont2);
                    } else {
                        System.out.println("Server is down or not responding");
                    }
                    }else if(nodeName.equals("air_sample") || nodeName.equals("env_sample"))
                    {
                        List<PairNameIp> ipsToContact = db.getIPs(filterDb);
                        String ipcont = null;

                        for (PairNameIp pair : ipsToContact) {
                            System.out.println("name e ip: " + pair.name + " " + pair.ip);
                            if(pair.name.equals(nodeName))
                            {
                                ipcont = pair.ip;
                            }
                        }
                        CoapClient client3 = new CoapClient("coap://["+ ipcont + "]:5683/shutdown");
                        System.out.println("RICHIEDO SHUTDOWN A coap://["+ipcont + "]:5683/shutdown");
                        CoapResponse response3 = client3.get();

                        if (response3 != null) {
                            System.out.println(nodeName + " is shutted\n");
                            //remove from ipv6_addresses the device ip
                            db.removeIp(ipcont);
                        } else {
                            System.out.println("Server is down or not responding");
                        }
                    }
                    //taking the ip of the node
                    
                    break;

                case 5:
                    // Exit
                    System.out.println("Exiting...");
                    System.exit(0);
                    break;
                    
            default:
                System.out.println("Invalid choice");
            }
        }
        catch (ConnectorException | IOException e) {
        System.out.println("Errore di comunicazione CoAP: " + e.getMessage());
        } catch (JsonException | NumberFormatException e) {
        System.out.println("Formato di risposta non valido: " + e.getMessage());
        }
        }
    }
}