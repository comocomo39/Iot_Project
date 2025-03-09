package com.unipi.dii.iot;

import java.sql.Connection;
import java.sql.DriverManager;
import java.sql.PreparedStatement;
import java.sql.ResultSet;
import java.sql.SQLException;
import java.sql.Statement;
import java.sql.Timestamp;
import java.util.ArrayList;
import java.util.List;

import org.json.simple.JSONArray;

import com.unipi.dii.iot.IPv6DatabaseManager.PairNameIp;

public class IPv6DatabaseManager {
    static final String JDBC_URL = "jdbc:mysql://localhost:3306/iotdatabase";
    static final String JDBC_USER = "root";
    static final String JDBC_PASSWORD = "PASSWORD";

    public static void createDatabase() {
        String JDBC_URL1 = "jdbc:mysql://localhost:3306/";
        String JDBC_USER1 = "root";
        String JDBC_PASSWORD1 = "PASSWORD";
        String DATABASE_NAME1 = "iotdatabase";
    
        String createDatabaseSQL = "CREATE DATABASE IF NOT EXISTS " + DATABASE_NAME1;
    
        try (Connection conn = DriverManager.getConnection(JDBC_URL1, JDBC_USER1, JDBC_PASSWORD1);
             Statement stmt = conn.createStatement()) {
            stmt.execute(createDatabaseSQL);
            System.out.println("Database created successfully.");
        } catch (SQLException e) {
            System.out.println("Database not created.");
            e.printStackTrace();
        }
    }
    
    static Connection connect() throws SQLException {
        return DriverManager.getConnection(JDBC_URL, JDBC_USER, JDBC_PASSWORD);
    }

    public static void createTableIPV6() {
        String createTableSQL = "CREATE TABLE IF NOT EXISTS ipv6_addresses (" +
                                "id INT AUTO_INCREMENT PRIMARY KEY, " +
                                "time TIMESTAMP DEFAULT CURRENT_TIMESTAMP, " +
                                "address VARCHAR(89) NOT NULL, " +
                                "type VARCHAR(80) NOT NULL," + 
                                "name VARCHAR(80) NOT NULL)";

        try (Connection conn = connect();
             Statement stmt = conn.createStatement()) {
            stmt.execute(createTableSQL);
        } catch (SQLException e) {
            e.printStackTrace();
        }
    }

    public void insertIPv6Address(String address, String type, String name) {
        createTableIPV6();
        
        String insertSQL = "INSERT INTO ipv6_addresses (address, type, name) VALUES (?, ?, ?)";

        try (Connection conn = connect();
             PreparedStatement pstmt = conn.prepareStatement(insertSQL)) {
            pstmt.setString(1, address);
            pstmt.setString(2, type);
            pstmt.setString(3, name);
            pstmt.executeUpdate();
        } catch (SQLException e) {
            e.printStackTrace();
        }
    }

    public List<String> fetchIPv6Addresses() {
        List<String> ipAddresses = new ArrayList<>();
        String querySQL = "SELECT address FROM ipv6_addresses";

        try (Connection conn = connect();
             Statement stmt = conn.createStatement();
             ResultSet rs = stmt.executeQuery(querySQL)) {

            while (rs.next()) {
                String address = rs.getString("address");
                ipAddresses.add(address);
            }
        } catch (SQLException e) {
            e.printStackTrace();
        }

        return ipAddresses;
    }
//utility class to store the name and ip of the sensor
public class PairNameIp {
    String ip;
    String name;

    public PairNameIp() {
        this.ip = null;
        this.name = null;
    }

    public PairNameIp(String ip, String name) {
        this.ip = ip;
        this.name = name;
    }
}
    

    public List<PairNameIp> getIPs(String type) {
        List<PairNameIp> sensorIPs = new ArrayList<>();
        String querySQL = "SELECT address, name FROM ipv6_addresses WHERE type = '" + type + "'";

        try (Connection conn = connect();
             PreparedStatement pstmt = conn.prepareStatement(querySQL);
             ResultSet rs = pstmt.executeQuery()) {
            while (rs.next()) {
                sensorIPs.add(new PairNameIp(rs.getString("address"),rs.getString("name")));
            }
        } catch (SQLException e) {
            e.printStackTrace();
        }

        return sensorIPs;
    }

    public static void deleteDB() {
        String deleteDBSQL = "DROP DATABASE IF EXISTS iotdatabase";

        try (Connection conn = DriverManager.getConnection(JDBC_URL, JDBC_USER, JDBC_PASSWORD);
             Statement stmt = conn.createStatement()) {
            stmt.execute(deleteDBSQL);
            System.out.println("Database deleted successfully.");
        } catch (SQLException e) {
            e.printStackTrace();
        }
    }
    public static void createTableSensor(String sensorName, String ip, JSONArray ss) {
        ip = ip.replace(":", "");

        String tableName = sensorName.toLowerCase() + "_" + ip; 
        String createTableColumns = "";
        for (int i = 0; i < ss.size(); i++) {
            System.out.println("ss.get( "+ i+ ").toString(): " + ss.get(i).toString());
            if(!ss.get(i).toString().equals("value") && !ss.get(i).toString().equals("ts")){
                createTableColumns += ss.get(i).toString() + " INT NOT NULL, ";
            }
        }
        String createTableSQL = "CREATE TABLE IF NOT EXISTS " + tableName + " ("
            + "id INT AUTO_INCREMENT, " 
            + "time TIMESTAMP DEFAULT CURRENT_TIMESTAMP, "
            + "sensorName VARCHAR(50) NOT NULL, "
            + createTableColumns 
            + "PRIMARY KEY (id)) ";

        try (Connection conn = connect();
             Statement stmt = conn.createStatement()) {
            stmt.execute(createTableSQL);
        } catch (SQLException e) {
            e.printStackTrace();
        }
    }

    public void insertSensorENV(String sensorName, String ip, JSONArray ss) {
        //checking if table exists, if not we create it
        JSONArray valuesArray = new JSONArray();
        valuesArray.add("temperature");
        valuesArray.add("humidity");
        valuesArray.add("dust_density");
        sensorName = sensorName.toLowerCase();
        createTableSensor(sensorName, ip, valuesArray);

        ip = ip.replace(":", "");
        String tableName = sensorName + "_" + ip;
        String insertSQL = "INSERT INTO " + tableName + " (sensorName, temperature, humidity, dust_density) VALUES (?, ?, ?, ?)";

        try (Connection conn = connect();
            PreparedStatement pstmt = conn.prepareStatement(insertSQL)) {
            pstmt.setString(1, sensorName);
            //casting value from jSon to int
            Long longValue = (Long) ss.get(0);
            pstmt.setInt(2, longValue.intValue());
            longValue = (Long) ss.get(1);
            pstmt.setInt(3, longValue.intValue());       
            pstmt.executeUpdate();
        } catch (SQLException e) {
            e.printStackTrace();
        }
    }

    public void insertSensorAIR(String sensorName, String ip, JSONArray ss) {
    // Checking if table exists, if not, we create it
    JSONArray valuesArray = new JSONArray();
    valuesArray.add("co");
    valuesArray.add("air_quality");
    valuesArray.add("tvoc");

    String sensorNameTable = sensorName.toLowerCase();
    createTableSensor(sensorNameTable, ip, valuesArray);

    // Formatting table name
    ip = ip.replace(":", "");
    String tableName = sensorNameTable + "_" + ip;
    String insertSQL = "INSERT INTO " + tableName + " (sensorName, co, air_quality, tvoc, prediction) VALUES (?, ?, ?, ?, ?)";

    try (Connection conn = connect();
         PreparedStatement pstmt = conn.prepareStatement(insertSQL)) {

        pstmt.setString(1, sensorName);

        // Ensure JSONArray has at least 3 values
        if (ss.size() >= 3) {
            pstmt.setInt(2, ((Long) ss.get(0)).intValue());
            pstmt.setInt(3, ((Long) ss.get(1)).intValue());
            pstmt.setInt(4, ((Long) ss.get(2)).intValue());
        } else {
            throw new IllegalArgumentException("JSONArray 'ss' must contain at least 3 elements.");
        }

        // Generate prediction value
        int predictionValue = (int) (Math.random() * 3) + 1;
        System.out.println("PREDICTION: " + predictionValue);
        pstmt.setInt(5, predictionValue);

        pstmt.executeUpdate();
        System.out.println("Sensor inserted successfully.");
    } catch (SQLException e) {
        e.printStackTrace();
    }
}


    public void insertActuatorVENTILATION(String name, String ip, float threshold, String state) {

        String tableName = name + "_" + ip.replace(":", "");
        String insertSQL = "INSERT INTO " + tableName + " (sensorName, ipAddress, threshold, state, timestamp) VALUES (?, ?, ?, ?, ?)";

        try (Connection conn = connect();
            PreparedStatement pstmt = conn.prepareStatement(insertSQL)) {
            pstmt.setString(1, name);
            pstmt.setString(2, ip);
            pstmt.setFloat(3, threshold);
            pstmt.setString(4, state);
            pstmt.setTimestamp(5, new Timestamp(System.currentTimeMillis()));
            pstmt.executeUpdate();
            System.out.println("Actuator inserted successfully.");
        } catch (SQLException e) {
            e.printStackTrace();
        }
    }


    public void insertActuatorAIRSYSTEM(String name, String ip, float threshold, String state) {

        String tableName = name + "_" + ip.replace(":", "");
        String insertSQL = "INSERT INTO " + tableName + " (sensorName, ipAddress, threshold, state, timestamp) VALUES (?, ?, ?, ?, ?)";

        try (Connection conn = connect();
            PreparedStatement pstmt = conn.prepareStatement(insertSQL)) {
            pstmt.setString(1, name);
            pstmt.setString(2, ip);
            pstmt.setFloat(3, threshold);
            pstmt.setString(4, state);
            pstmt.setTimestamp(5, new Timestamp(System.currentTimeMillis()));
            pstmt.executeUpdate();
        } catch (SQLException e) {
            e.printStackTrace();
        }
    }

    public Boolean checkAllSensorsRegistered() {
        List<PairNameIp> sensorIPs = getIPs("sensor");

        return sensorIPs.size() >= 2;
    }
}