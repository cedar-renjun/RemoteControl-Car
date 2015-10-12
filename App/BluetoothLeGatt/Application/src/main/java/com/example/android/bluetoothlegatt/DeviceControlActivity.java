/*
 * Copyright (C) 2013 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package com.example.android.bluetoothlegatt;

import android.app.Activity;
import android.bluetooth.BluetoothGattCharacteristic;
import android.bluetooth.BluetoothGattService;
import android.content.BroadcastReceiver;
import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.content.ServiceConnection;
import android.hardware.Sensor;
import android.hardware.SensorEvent;
import android.hardware.SensorEventListener;
import android.hardware.SensorManager;
import android.os.Bundle;
import android.os.IBinder;
import android.os.PowerManager;
import android.util.Log;
import android.view.Menu;
import android.view.MenuItem;
import android.view.View;
import android.widget.CompoundButton;
import android.widget.ExpandableListView;
import android.widget.SeekBar;
import android.widget.SimpleExpandableListAdapter;
import android.widget.Switch;
import android.widget.TextView;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;

/**
 * For a given BLE device, this Activity provides the user interface to connect, display data,
 * and display GATT services and characteristics supported by the device.  The Activity
 * communicates with {@code BluetoothLeService}, which in turn interacts with the
 * Bluetooth LE API.
 */
public class DeviceControlActivity extends Activity implements SeekBar.OnSeekBarChangeListener ,SensorEventListener {
    private final static String TAG = DeviceControlActivity.class.getSimpleName();

    public static final String EXTRAS_DEVICE_NAME = "DEVICE_NAME";
    public static final String EXTRAS_DEVICE_ADDRESS = "DEVICE_ADDRESS";

    public static String CLIENT_CHARACTERISTIC_MOTOR = "0000fff1-0000-1000-8000-00805f9b34fb";

    private TextView mConnectionState;
    private TextView mDataField;
    private String mDeviceName;
    private String mDeviceAddress;
    private ExpandableListView mGattServicesList;
    private BluetoothLeService mBluetoothLeService;
    private ArrayList<ArrayList<BluetoothGattCharacteristic>> mGattCharacteristics =
            new ArrayList<ArrayList<BluetoothGattCharacteristic>>();
    private boolean mConnected = false;
    private BluetoothGattCharacteristic mNotifyCharacteristic;

    private final String LIST_NAME = "NAME";
    private final String LIST_UUID = "UUID";

    private byte[] value = new byte[8];

    private boolean motor_run = false;
    private TextView mShowSpeed0,mShowSpeed1,mShowSpeed2,mShowSpeed3;
    private SeekBar mSetSpeed0,mSetSpeed1,mSetSpeed2,mSetSpeed3;
    private BluetoothGattCharacteristic mMotorChars;

    private SensorManager mSensorManager;

    private Switch mDevState = null;
    private Switch mConState = null;
    private Switch mCtlState = null;

    private PowerManager pm = null;
    PowerManager.WakeLock wl = null;


    // Code to manage Service lifecycle.
    private final ServiceConnection mServiceConnection = new ServiceConnection() {

        @Override
        public void onServiceConnected(ComponentName componentName, IBinder service) {
            mBluetoothLeService = ((BluetoothLeService.LocalBinder) service).getService();
            if (!mBluetoothLeService.initialize()) {
                Log.e(TAG, "Unable to initialize Bluetooth");
                finish();
            }
            // Automatically connects to the device upon successful start-up initialization.
            mBluetoothLeService.connect(mDeviceAddress);
        }

        @Override
        public void onServiceDisconnected(ComponentName componentName) {
            mBluetoothLeService = null;
        }
    };

    // Handles various events fired by the Service.
    // ACTION_GATT_CONNECTED: connected to a GATT server.
    // ACTION_GATT_DISCONNECTED: disconnected from a GATT server.
    // ACTION_GATT_SERVICES_DISCOVERED: discovered GATT services.
    // ACTION_DATA_AVAILABLE: received data from the device.  This can be a result of read
    //                        or notification operations.
    private final BroadcastReceiver mGattUpdateReceiver = new BroadcastReceiver() {
        @Override
        public void onReceive(Context context, Intent intent) {
            final String action = intent.getAction();
            if (BluetoothLeService.ACTION_GATT_CONNECTED.equals(action)) {
                mConnected = true;
                updateConnectionState(R.string.connected);
                invalidateOptionsMenu();
            } else if (BluetoothLeService.ACTION_GATT_DISCONNECTED.equals(action)) {
                mConnected = false;
                updateConnectionState(R.string.disconnected);
                invalidateOptionsMenu();
                clearUI();
            } else if (BluetoothLeService.ACTION_GATT_SERVICES_DISCOVERED.equals(action)) {
                // Show all the supported services and characteristics on the user interface.
                displayGattServices(mBluetoothLeService.getSupportedGattServices());
            } else if (BluetoothLeService.ACTION_DATA_AVAILABLE.equals(action)) {
                displayData(intent.getStringExtra(BluetoothLeService.EXTRA_DATA));
            }
        }
    };

    // If a given GATT characteristic is selected, check for supported features.  This sample
    // demonstrates 'Read' and 'Notify' features.  See
    // http://d.android.com/reference/android/bluetooth/BluetoothGatt.html for the complete
    // list of supported characteristic features.
    private final ExpandableListView.OnChildClickListener servicesListClickListner =
            new ExpandableListView.OnChildClickListener() {
                @Override
                public boolean onChildClick(ExpandableListView parent, View v, int groupPosition,
                                            int childPosition, long id) {
                    if (mGattCharacteristics != null) {
                        final BluetoothGattCharacteristic characteristic =
                                mGattCharacteristics.get(groupPosition).get(childPosition);


//                        final int charaProp = characteristic.getProperties();
//                        if ((charaProp | BluetoothGattCharacteristic.PROPERTY_READ) > 0) {
//                            // If there is an active notification on a characteristic, clear
//                            // it first so it doesn't update the data field on the user interface.
//                            if (mNotifyCharacteristic != null) {
//                                mBluetoothLeService.setCharacteristicNotification(
//                                        mNotifyCharacteristic, false);
//                                mNotifyCharacteristic = null;
//                            }
//                            mBluetoothLeService.readCharacteristic(characteristic);
//                        }
//                        if ((charaProp | BluetoothGattCharacteristic.PROPERTY_NOTIFY) > 0) {
//                            mNotifyCharacteristic = characteristic;
//                            mBluetoothLeService.setCharacteristicNotification(
//                                    characteristic, true);
//                        }

                        if(characteristic.getUuid().toString().equals(CLIENT_CHARACTERISTIC_MOTOR))
                        {
                            mMotorChars = characteristic;

                            value[0] = 0x04;
                            value[5] += 10;
                            value[5] %= 100;

                            value[6] += 10;
                            value[6] %= 100;

                            characteristic.setValue(value);
                            mBluetoothLeService.writeCharacteristic(characteristic);

                            Log.d("BLE_TEST", "Write Begin");
                            Log.d("BLE_TEST", "value" + value[7]);

                        }

                        return true;
                    }
                    return false;
                }
    };

    private void clearUI() {
        mGattServicesList.setAdapter((SimpleExpandableListAdapter) null);
        mDataField.setText(R.string.no_data);
    }

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        setContentView(R.layout.gatt_services_characteristics);

        mShowSpeed0 = (TextView) findViewById(R.id.ShowSpeed0);
        mSetSpeed0  = (SeekBar) findViewById(R.id.SetSpeed0);
        mShowSpeed1 = (TextView) findViewById(R.id.ShowSpeed1);
        mSetSpeed1  = (SeekBar) findViewById(R.id.SetSpeed1);
        mShowSpeed2 = (TextView) findViewById(R.id.ShowSpeed2);
        mSetSpeed2  = (SeekBar) findViewById(R.id.SetSpeed2);
        mShowSpeed3 = (TextView) findViewById(R.id.ShowSpeed3);
        mSetSpeed3  = (SeekBar) findViewById(R.id.SetSpeed3);

        mDevState = (Switch) findViewById(R.id.DevState);
        mConState = (Switch) findViewById(R.id.ConState);
        mCtlState = (Switch) findViewById(R.id.CtlState);

//        mDevState.setEnabled(false);  // 玩家模式
//        mConState.setEnabled(true);   // 连接状态
//        mCtlState.setEnabled(false);  // 停止控制

        // 开发者模式 or 玩家模式
        mDevState.setOnCheckedChangeListener(new CompoundButton.OnCheckedChangeListener() {
            @Override
            public void onCheckedChanged(CompoundButton compoundButton, boolean b) {
                SetPlayMode(b);
            }
        });

        // 连接 or 断开
        mConState.setOnCheckedChangeListener(new CompoundButton.OnCheckedChangeListener() {
            @Override
            public void onCheckedChanged(CompoundButton compoundButton, boolean b) {
                // TODO
                mBluetoothLeService.disconnect();
                AppRun();
            }
        });

        // 开始控制 or 停止控制
        mCtlState.setOnCheckedChangeListener(new CompoundButton.OnCheckedChangeListener() {
            @Override
            public void onCheckedChanged(CompoundButton compoundButton, boolean b) {
                // TODO
            }
        });

        mShowSpeed0.setText("Ch0 PWM Duty " + 0);
        mSetSpeed0.setMax(100);
        mSetSpeed0.setProgress(0);
        mSetSpeed0.setOnSeekBarChangeListener(this);

        mShowSpeed1.setText("Ch1 PWM Duty " + 0);
        mSetSpeed1.setMax(100);
        mSetSpeed1.setProgress(0);
        mSetSpeed1.setOnSeekBarChangeListener(this);

        mShowSpeed2.setText("Ch2 PWM Duty " + 0);
        mSetSpeed2.setMax(100);
        mSetSpeed2.setProgress(0);
        mSetSpeed2.setOnSeekBarChangeListener(this);

        mShowSpeed3.setText("Ch3 PWM Duty " + 0);
        mSetSpeed3.setMax(100);
        mSetSpeed3.setProgress(0);
        mSetSpeed3.setOnSeekBarChangeListener(this);

        mSensorManager = (SensorManager) getSystemService(SENSOR_SERVICE);
        mSensorManager.registerListener(this,
                mSensorManager.getDefaultSensor(Sensor.TYPE_ACCELEROMETER),
                //SensorManager.SENSOR_DELAY_NORMAL);
                50000);

        final Intent intent = getIntent();
        mDeviceName = intent.getStringExtra(EXTRAS_DEVICE_NAME);
        mDeviceAddress = intent.getStringExtra(EXTRAS_DEVICE_ADDRESS);

        // Sets up UI references.
        ((TextView) findViewById(R.id.device_address)).setText(mDeviceAddress);
        mGattServicesList = (ExpandableListView) findViewById(R.id.gatt_services_list);
        mGattServicesList.setOnChildClickListener(servicesListClickListner);
        mConnectionState = (TextView) findViewById(R.id.connection_state);
        mDataField = (TextView) findViewById(R.id.data_value);

        getActionBar().setTitle(mDeviceName);
        getActionBar().setDisplayHomeAsUpEnabled(true);
        Intent gattServiceIntent = new Intent(this, BluetoothLeService.class);
        bindService(gattServiceIntent, mServiceConnection, BIND_AUTO_CREATE);

        SetPlayMode(false);
    }

    private void AppRun() {
        Intent intent = new Intent(this, DeviceScanActivity.class);
        startActivity(intent);
        finish();
    }

    private
    void SetPlayMode(boolean mode)
    {
        // true----developer mode
        // false---player mode
        TextView mText = null;

        if (true == mode) {
            // Address
            mText = (TextView) findViewById(R.id.device_address_0);
            mText.setVisibility(TextView.VISIBLE);

            mText = (TextView) findViewById(R.id.device_address);
            mText.setVisibility(TextView.VISIBLE);

            // Connect State
            mText = (TextView) findViewById(R.id.connection_state_0);
            mText.setVisibility(TextView.VISIBLE);

            mText = (TextView) findViewById(R.id.connection_state);
            mText.setVisibility(TextView.VISIBLE);

            // Data
            mText = (TextView) findViewById(R.id.data_value_0);
            mText.setVisibility(TextView.VISIBLE);

            mText = (TextView) findViewById(R.id.data_value);
            mText.setVisibility(TextView.VISIBLE);

            ExpandableListView mList = (ExpandableListView) findViewById(R.id.gatt_services_list);
            mList.setVisibility(ExpandableListView.VISIBLE);

        } else {
            // Address
            mText = (TextView) findViewById(R.id.device_address_0);
            mText.setVisibility(TextView.INVISIBLE);

            mText = (TextView) findViewById(R.id.device_address);
            mText.setVisibility(TextView.INVISIBLE);

            // Connect State
            mText = (TextView) findViewById(R.id.connection_state_0);
            mText.setVisibility(TextView.INVISIBLE);

            mText = (TextView) findViewById(R.id.connection_state);
            mText.setVisibility(TextView.INVISIBLE);

            // Data
            mText = (TextView) findViewById(R.id.data_value_0);
            mText.setVisibility(TextView.INVISIBLE);

            mText = (TextView) findViewById(R.id.data_value);
            mText.setVisibility(TextView.INVISIBLE);

            ExpandableListView mList = (ExpandableListView) findViewById(R.id.gatt_services_list);
            mList.setVisibility(ExpandableListView.INVISIBLE);
        }
    }



    @Override
    public void onProgressChanged(SeekBar var1, int var2, boolean var3) {
        /*
        value[0] = 0x04;
        if(mSetSpeed0 == var1) {
            value[4] = (byte) var1.getProgress();
            mShowSpeed0.setText("Ch0 PWM Duty " + var1.getProgress());
        }
        if(mSetSpeed1 == var1) {
            value[5] = (byte) var1.getProgress();
            mShowSpeed1.setText("Ch1 PWM Duty " + var1.getProgress());
        }
        if(mSetSpeed2 == var1) {
            value[6] = (byte) var1.getProgress();
            mShowSpeed2.setText("Ch2 PWM Duty " + var1.getProgress());
        }
        if(mSetSpeed3 == var1) {
            value[7] = (byte) var1.getProgress();
            mShowSpeed3.setText("Ch3 PWM Duty " + var1.getProgress());
        }
        */
        //mMotorChars.setValue(value);
        //mBluetoothLeService.writeCharacteristic(mMotorChars);
    }

    @Override
    public void onStartTrackingTouch(SeekBar var1){

    }

    @Override
    public void onStopTrackingTouch(SeekBar var1){

    }

    @Override
    public void onSensorChanged(SensorEvent var1){
        if (var1.sensor.getType() == Sensor.TYPE_ACCELEROMETER) {
            float[] values = var1.values;

//            Log.d("BLE_SENSOR", "Length" + values.length);
//            for(int i = 0; i < values.length; i++) {
//                Log.d("BLE_SENSOR", "value " + i + values[0]);
//            }

            // Control Car
            int speed_fb =  (int)(-20*values[1]);
            int speed_rl =  (int)(20*values[0]);

            //mShowSpeed0.setText("X Acc value " + speed_fb);
            //mShowSpeed1.setText("Y Acc value " + speed_rl);

            if(false == mCtlState.isChecked()){
                RemoteCar(0, 0);
            }
            else {
                RemoteCar(speed_fb, speed_rl);
            }
        }
    }

    // Forward-Back +F-B
    // Right-Left   +L-R
    void RemoteCar(int speed_fb, int speed_rl){
        int speed_l = speed_fb+speed_rl/2;
        int speed_r = speed_fb-speed_rl/2;

        for(int i = 0; i < value.length; i++)
        {
            value[i] = 0;
        }

        if(speed_l >  100) speed_l =  100;
        if(speed_l < -100) speed_l = -100;
        if(speed_r >  100) speed_r =  100;
        if(speed_r < -100) speed_r = -100;

        value[0] = 0x04;
        if(speed_l > 0) {
            value[4] = (byte)speed_l;
            value[5] = 0;
        }
        else if(speed_l < 0){
            value[4] = 0;
            value[5] = (byte)-speed_l;
        }
        else {
            // Do nothing
        }

        if(speed_r < 0) {
            value[6] = (byte)-speed_r;
            value[7] = 0;
        }
        else if(speed_r > 0){
            value[6] = 0;
            value[7] = (byte)speed_r;
        }
        else {
            // Do nothing
        }

        ///////

        mShowSpeed0.setText("Ch0 PWM Duty " + value[4]);
        mShowSpeed1.setText("Ch1 PWM Duty " + value[5]);
        mShowSpeed2.setText("Ch2 PWM Duty " + value[6]);
        mShowSpeed3.setText("Ch3 PWM Duty " + value[7]);

        mSetSpeed0.setProgress(value[4]);
        mSetSpeed1.setProgress(value[5]);
        mSetSpeed2.setProgress(value[6]);
        mSetSpeed3.setProgress(value[7]);

//        int progress = (int)value[4];
//        mSetSpeed0.setProgress(progress);
//        progress = (int)value[5];
//        mSetSpeed1.setProgress(progress);
//        progress = (int)value[6];
//        mSetSpeed2.setProgress(progress);
//        progress = (int)value[7];
//        mSetSpeed3.setProgress(progress);

        if(null != mMotorChars) {
            mMotorChars.setValue(value);
            if(null != mBluetoothLeService)
            mBluetoothLeService.writeCharacteristic(mMotorChars);
        }
        else
        {
            Log.d("BLE_SENSOR", "Error");
        }

        Log.d("BLE_SENSOR", "Update");
        ///////
    }

    @Override
    public void onAccuracyChanged(Sensor var1, int var2){
        // Do Nothing
    }

    @Override
    protected void onResume() {
        super.onResume();
        if(null == pm) {
            pm = (PowerManager) getSystemService(Context.POWER_SERVICE);
            wl = pm.newWakeLock(PowerManager.SCREEN_BRIGHT_WAKE_LOCK, "My Tag");
            wl.acquire();
        }

        registerReceiver(mGattUpdateReceiver, makeGattUpdateIntentFilter());
        if (mBluetoothLeService != null) {
            final boolean result = mBluetoothLeService.connect(mDeviceAddress);
            Log.d(TAG, "Connect request result=" + result);
        }

        if(null != mSensorManager) {
            mSensorManager.registerListener(this,
                    mSensorManager.getDefaultSensor(Sensor.TYPE_ACCELEROMETER),
                    //SensorManager.SENSOR_DELAY_NORMAL);
                    2000000);
        }

    }

    @Override
    protected void onPause() {
        super.onPause();
        unregisterReceiver(mGattUpdateReceiver);
        mSensorManager.unregisterListener(this);
        if(null != wl) {
            wl.release();
            wl = null;
        }
    }

    @Override
    protected void onDestroy() {
        super.onDestroy();
        unbindService(mServiceConnection);
        mBluetoothLeService = null;
    }

    @Override
    public boolean onCreateOptionsMenu(Menu menu) {
        getMenuInflater().inflate(R.menu.gatt_services, menu);
        if (mConnected) {
            menu.findItem(R.id.menu_connect).setVisible(false);
            menu.findItem(R.id.menu_disconnect).setVisible(true);
        } else {
            menu.findItem(R.id.menu_connect).setVisible(true);
            menu.findItem(R.id.menu_disconnect).setVisible(false);
        }
        return true;
    }

    @Override
    public boolean onOptionsItemSelected(MenuItem item) {
        switch(item.getItemId()) {
            case R.id.menu_connect:
                mBluetoothLeService.connect(mDeviceAddress);
                return true;
            case R.id.menu_disconnect:
                mBluetoothLeService.disconnect();
                return true;
            case android.R.id.home:
                onBackPressed();
                return true;
        }
        return super.onOptionsItemSelected(item);
    }

    private void updateConnectionState(final int resourceId) {
        runOnUiThread(new Runnable() {
            @Override
            public void run() {
                mConnectionState.setText(resourceId);
            }
        });
    }

    private void displayData(String data) {
        if (data != null) {
            mDataField.setText(data);
        }
    }

    // Demonstrates how to iterate through the supported GATT Services/Characteristics.
    // In this sample, we populate the data structure that is bound to the ExpandableListView
    // on the UI.
    private void displayGattServices(List<BluetoothGattService> gattServices) {
        if (gattServices == null) return;
        String uuid = null;
        String unknownServiceString = getResources().getString(R.string.unknown_service);
        String unknownCharaString = getResources().getString(R.string.unknown_characteristic);
        ArrayList<HashMap<String, String>> gattServiceData = new ArrayList<HashMap<String, String>>();
        ArrayList<ArrayList<HashMap<String, String>>> gattCharacteristicData
                = new ArrayList<ArrayList<HashMap<String, String>>>();
        mGattCharacteristics = new ArrayList<ArrayList<BluetoothGattCharacteristic>>();

        // Loops through available GATT Services.
        for (BluetoothGattService gattService : gattServices) {
            HashMap<String, String> currentServiceData = new HashMap<String, String>();
            uuid = gattService.getUuid().toString();
            currentServiceData.put(
                    LIST_NAME, SampleGattAttributes.lookup(uuid, unknownServiceString));
            currentServiceData.put(LIST_UUID, uuid);
            gattServiceData.add(currentServiceData);

            ArrayList<HashMap<String, String>> gattCharacteristicGroupData =
                    new ArrayList<HashMap<String, String>>();
            List<BluetoothGattCharacteristic> gattCharacteristics =
                    gattService.getCharacteristics();
            ArrayList<BluetoothGattCharacteristic> charas =
                    new ArrayList<BluetoothGattCharacteristic>();

            // Loops through available Characteristics.
            for (BluetoothGattCharacteristic gattCharacteristic : gattCharacteristics) {
                charas.add(gattCharacteristic);
                HashMap<String, String> currentCharaData = new HashMap<String, String>();
                uuid = gattCharacteristic.getUuid().toString();
                currentCharaData.put(
                        LIST_NAME, SampleGattAttributes.lookup(uuid, unknownCharaString));
                currentCharaData.put(LIST_UUID, uuid);
                gattCharacteristicGroupData.add(currentCharaData);

                // Set
                if(gattCharacteristic.getUuid().toString().equals(CLIENT_CHARACTERISTIC_MOTOR)) {
                    mMotorChars = gattCharacteristic;
                }
            }
            mGattCharacteristics.add(charas);
            gattCharacteristicData.add(gattCharacteristicGroupData);
        }

        SimpleExpandableListAdapter gattServiceAdapter = new SimpleExpandableListAdapter(
                this,
                gattServiceData,
                android.R.layout.simple_expandable_list_item_2,
                new String[] {LIST_NAME, LIST_UUID},
                new int[] { android.R.id.text1, android.R.id.text2 },
                gattCharacteristicData,
                android.R.layout.simple_expandable_list_item_2,
                new String[] {LIST_NAME, LIST_UUID},
                new int[] { android.R.id.text1, android.R.id.text2 }
        );
        mGattServicesList.setAdapter(gattServiceAdapter);
    }

    private static IntentFilter makeGattUpdateIntentFilter() {
        final IntentFilter intentFilter = new IntentFilter();
        intentFilter.addAction(BluetoothLeService.ACTION_GATT_CONNECTED);
        intentFilter.addAction(BluetoothLeService.ACTION_GATT_DISCONNECTED);
        intentFilter.addAction(BluetoothLeService.ACTION_GATT_SERVICES_DISCOVERED);
        intentFilter.addAction(BluetoothLeService.ACTION_DATA_AVAILABLE);
        return intentFilter;
    }
}
