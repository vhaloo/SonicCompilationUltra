package com.vhaloo.soniccompilationultra.mobile;

import android.content.Context;
import android.hardware.Sensor;
import android.hardware.SensorEvent;
import android.hardware.SensorEventListener;
import android.hardware.SensorManager;
import java.util.function.IntSupplier;

/** Optional 50 Hz motion source. Game rotation uses Android's gyro/accel fusion;
 * gravity and filtered acceleration provide fallbacks. Nothing is recorded. */
final class MotionSensors implements SensorEventListener {
    interface Sink {
        void sample(float x,float y,float z,float w,float gx,float gy,float gz,
                    float ax,float ay,float az,int source,int rotation);
    }
    private final SensorManager manager;
    private final Sensor posture,linear,accelerometer;
    private final Sink sink;
    private final IntSupplier rotation;
    private final int source;
    private final float[] quaternion=new float[]{1,0,0,0},gravity=new float[3],force=new float[3];
    private boolean running=false,gestures=false,poseReady=false,gravityReady=false;
    MotionSensors(Context context,Sink sink,IntSupplier rotation) {
        this.sink=sink;this.rotation=rotation;
        manager=(SensorManager)context.getSystemService(Context.SENSOR_SERVICE);
        Sensor chosen=find(Sensor.TYPE_GAME_ROTATION_VECTOR);
        if(chosen==null)chosen=find(Sensor.TYPE_ROTATION_VECTOR);
        int kind=1;
        if(chosen==null){chosen=find(Sensor.TYPE_GRAVITY);kind=2;}
        if(chosen==null){chosen=find(Sensor.TYPE_ACCELEROMETER);kind=3;}
        posture=chosen;source=chosen==null?0:kind;
        linear=find(Sensor.TYPE_LINEAR_ACCELERATION);
        accelerometer=find(Sensor.TYPE_ACCELEROMETER);
    }
    private Sensor find(int type){return manager==null?null:manager.getDefaultSensor(type);}
    int source(){return source;}
    void active(boolean enabled,boolean flick) {
        if(enabled==running&&flick==gestures)return;
        stop();gestures=flick;
        if(!enabled||posture==null)return;
        try {
            running=manager.registerListener(this,posture,20000,0);
            if(running&&flick) {
                if(linear!=null)manager.registerListener(this,linear,20000,0);
                else if(accelerometer!=null&&accelerometer!=posture)
                    manager.registerListener(this,accelerometer,20000,0);
            }
        } catch(SecurityException unavailable){stop();}
    }
    void stop() {
        if(manager!=null)manager.unregisterListener(this);
        running=poseReady=gravityReady=false;
        for(int i=0;i<3;i++){gravity[i]=0;force[i]=0;}
        sink.sample(0,0,0,1,0,0,0,0,0,0,0,rotation.getAsInt());
    }
    @Override public void onSensorChanged(SensorEvent event) {
        if(!running||event.values.length<3)return;
        for(int i=0;i<3;i++)if(!Float.isFinite(event.values[i]))return;
        final int type=event.sensor.getType();
        if(type==Sensor.TYPE_GAME_ROTATION_VECTOR||type==Sensor.TYPE_ROTATION_VECTOR) {
            SensorManager.getQuaternionFromVector(quaternion,event.values);poseReady=true;
        } else if(type==Sensor.TYPE_GRAVITY) {
            System.arraycopy(event.values,0,gravity,0,3);poseReady=true;
        } else if(type==Sensor.TYPE_LINEAR_ACCELERATION) {
            System.arraycopy(event.values,0,force,0,3);
        } else if(type==Sensor.TYPE_ACCELEROMETER) {
            for(int i=0;i<3;i++) {
                gravity[i]=gravityReady?gravity[i]*.9f+event.values[i]*.1f:event.values[i];
                if(linear==null)force[i]=event.values[i]-gravity[i];
            }
            gravityReady=true;if(posture==accelerometer)poseReady=true;
        }
        if(poseReady)sink.sample(quaternion[1],quaternion[2],quaternion[3],quaternion[0],
            gravity[0],gravity[1],gravity[2],gestures?force[0]:0,gestures?force[1]:0,
            gestures?force[2]:0,source,rotation.getAsInt());
    }
    @Override public void onAccuracyChanged(Sensor sensor,int accuracy) {}
}
