
// https://github.com/Eugene-E0a80fd8080ff8e/


using System;
using UnityEngine;
using UnityEngine.UI;

using System.Collections;
using System.Collections.Generic;
using System.Text.RegularExpressions;
using System.Linq;

using System.IO.Pipes;

using System.Net.Sockets;
using System.Threading;
using System.Net;

using System.Text;
using System.Security.Principal;

namespace MVRPlugin
{

    public class HeadTracker : MVRScript
    {

        Vector3 headPosition;
        Quaternion headRotation;
        int headTimeRec;
        int headTimeSens;

        Vector3 headPosition2;
        Quaternion headRotation2;
        int headTimeRec2;
        int headTimeSens2;
        

        float amplification_x = 2, amplification_y = 2, amplification_z = 1;
        float limiter_x = 20, limiter_y = 45, limiter_z = 25;
        float damper_ms = 100;
        float finalDamper_ms = 200;
        float deadbandAngle = 3;

        Func<Quaternion, int,int, Quaternion> linearPredictor = null;
        Func<Quaternion, Quaternion> damper = null, limiter=null, amplifier=null, deadband=null, finalDamper = null;

        #region calculations itself

        Quaternion calc()
        {
            //return headRotation; //bno055


            if (headTimeSens - headTimeSens2 < 200 && headTimeSens - headTimeSens2 != 0 && headTimeRec - headTimeRec2 < 200 && Environment.TickCount - headTimeRec < 200)
            { }  // all ok
            else
            {
                // sensor stale, reset
                damper = null;
                deadband = null;
                finalDamper = null;
                linearPredictor = null;
                //return; 
            }

            // linear predictor. It is needed to compensate for differencies between sensor rate and frame rate.
            
            if( null == linearPredictor)
            {
                int prev2_sensorTime = -1;
                int prev2_recTime = -1;
                Quaternion prev2_q = Quaternion.identity;

                int prev_sensorTime = -1;
                int prev_recTime = -1;
                Quaternion prev_q = Quaternion.identity;
                linearPredictor = (q, sensorTime, recTime) =>
                {
                    if (prev_sensorTime == -1)
                    {
                        prev2_sensorTime = prev_sensorTime;
                        prev2_recTime = prev_recTime;
                        prev2_q = prev_q;
                        prev_sensorTime = sensorTime;
                        prev_recTime = recTime;
                        prev_q = q;
                        return q;
                    }
                    else if (prev_sensorTime == sensorTime) // the sensor have not since the last frame
                    {
                        float timeAdv = ((Environment.TickCount - prev_recTime) + (prev_sensorTime - prev2_sensorTime)) / (float)(prev_sensorTime - prev2_sensorTime);
                        //d($"timeAdv = {timeAdv} ; NU");
                        Quaternion res = Quaternion.SlerpUnclamped(prev_q, q, timeAdv);
                        return res;
                    }
                    else
                    {
                        float timeAdv = ((Environment.TickCount - recTime) + (sensorTime - prev_sensorTime)) / (float)(sensorTime - prev_sensorTime);
                        //d($"timeAdv = {timeAdv}");
                        Quaternion res = Quaternion.SlerpUnclamped(prev_q, q, timeAdv);

                        prev2_sensorTime = prev_sensorTime;
                        prev2_recTime = prev_recTime;
                        prev2_q = prev_q;
                        prev_sensorTime = sensorTime;
                        prev_recTime = recTime;
                        prev_q = q;

                        return res;
                    }

                };
            }

            if (null == damper)
            {
                Quaternion prev_q = Quaternion.identity;
                int prev_t = -1; 
                damper = (q) => // capturing prev_q and prev_t and damper_ms
                {
                    if( prev_t == -1)
                    {
                        prev_q = q;
                        prev_t = Environment.TickCount;
                        return q;
                    }
                    int t = Environment.TickCount;
                    float p = Mathf.Exp( -(t - prev_t) / damper_ms );

                    d($"damper_ms={damper_ms}");

                    Quaternion res;
                    if (p < 1e-8) res = q;
                    else res = Quaternion.Slerp(q, prev_q, p);
                    prev_q = res;
                    prev_t = t;

                    return res;
                };
            }

            if(null == limiter)
            {
                limiter = (q) =>
                {
                    Vector3 a = q.eulerAngles;
                    
                    if (a.x > 180) a.x -= 360;
                    if (a.y > 180) a.y -= 360;
                    if (a.z > 180) a.z -= 360;
                    //d($"limiter : {a.x} , {a.y} , {a.z}");
                    a.x = Mathf.Clamp(a.x, -limiter_x, limiter_x);
                    a.y = Mathf.Clamp(a.y, -limiter_y, limiter_y);
                    a.z = Mathf.Clamp(a.z, -limiter_z, limiter_z);
                    
                    return Quaternion.Euler(a);
                };
            }

            if(null == deadband)
            {
                bool deadbandNeedsInit = true;
                Quaternion prev_used_q = Quaternion.identity;
                deadband = (q) =>
                {
                    if(deadbandNeedsInit)
                    {
                        prev_used_q = q;
                        deadbandNeedsInit = false;
                        return q;
                    }
                    float angle = Quaternion.Angle(prev_used_q, q);
                    if (angle > deadbandAngle)
                        prev_used_q = q;
                    return prev_used_q;
                };
            }

            if (null == finalDamper)
            {
                Quaternion prev_q = Quaternion.identity;
                int prev_t = -1;
                finalDamper = (q) => // capturing prev_q and prev_t and damper_ms
                {
                    if(prev_t == -1)
                    {
                        prev_t = Environment.TickCount;
                        prev_q = q;
                        return q;
                    }
                    int t = Environment.TickCount;
                    float p = Mathf.Exp(-(t - prev_t) / finalDamper_ms);
                    Quaternion res;
                    if (p < 1e-8) res = q;
                    else res = Quaternion.Slerp(q, prev_q, p);
                    prev_q = res;
                    prev_t = t;

                    return res;
                };
            }

            if (null == amplifier)
            {
                amplifier = (q) => 
                {
                    Vector3 a = q.eulerAngles;

                    if (a.x > 180) a.x -= 360;
                    if (a.y > 180) a.y -= 360;
                    if (a.z > 180) a.z -= 360;

                    //d($"amplification_x = {amplification_x}");
                    a.x *= amplification_x;
                    a.y *= amplification_y;
                    a.z *= amplification_z;

                    return Quaternion.Euler(a);
                };
            }

            Quaternion rot = headRotation;

            rot = damper(rot); // stateful and time-dependent
            rot = linearPredictor(rot, headTimeSens, headTimeRec); // stateful and time-dependent
            //rot = deadband(rot); // stateful
            //rot = limiter(rot); // stateless
            rot = finalDamper(rot); // stateful and time-dependent
            rot = amplifier(rot); // stateless

            return rot;

        }

        #endregion

        #region decoding data received from UDP

        private void processPacket(byte[] p, int millis)
        {
            int pp = 0;
            while (pp < p.Length)
            {
                byte sensorType = p[pp];
                pp += 1;
                switch (sensorType)
                {
                    case 67:
                        {
                            int dataSize = System.BitConverter.ToInt16(p, pp);
                            pp += 2;

                            Int64 timestamp = System.BitConverter.ToInt64(p, pp);
                            pp += 8;

                            float x = (float)System.BitConverter.ToDouble(p, pp);
                            pp += 8;
                            float y = (float)System.BitConverter.ToDouble(p, pp);
                            pp += 8;
                            float z = (float)System.BitConverter.ToDouble(p, pp);
                            pp += 8;

                            float a = (float)System.BitConverter.ToDouble(p, pp);
                            pp += 8;
                            float b = (float)System.BitConverter.ToDouble(p, pp);
                            pp += 8;
                            float c = (float)System.BitConverter.ToDouble(p, pp);
                            pp += 8;


                            headTimeSens2 = headTimeSens;
                            headTimeRec2 = headTimeRec;
                            headPosition2 = headPosition;
                            headRotation2 = headRotation;

                            headTimeRec = millis;  // time when packet was received , local time
                            headTimeSens = (int)timestamp;  // when pose sensor produced this data, sensor time

                            headPosition = new Vector3(x, y, z);

                            {
                                Vector3 rodr = new Vector3(a, b, c);
                                float angle = rodr.magnitude;
                                Quaternion headLatestQuaternion = Quaternion.AngleAxis(angle * 180 / Mathf.PI, rodr / angle);

                                Vector3 euler = headLatestQuaternion.eulerAngles;

                                float euler_x = euler.x;
                                float euler_y = euler.y;
                                float euler_z = euler.z;

                                if (euler_x > 180) euler_x -= 360;
                                if (euler_y > 180) euler_y -= 360;
                                euler_x *= -1f;
                                euler_y *= -1f;
                                euler_z -= 180;
                                euler_z *= -1f;

                                headRotation = Quaternion.Euler(euler_x, euler_y, euler_z);
                            }

                            break;
                        }
                    case 68:
                        {
                            int dataSize = System.BitConverter.ToInt16(p, pp);
                            pp += 2;

                            Int32 timestamp = System.BitConverter.ToInt32(p, pp);
                            pp += 4;

                            Int32 counter = System.BitConverter.ToInt32(p, pp);
                            pp += 4;

                            float w = (float)System.BitConverter.ToInt16(p, pp);
                            pp += 2;
                            float x = (float)System.BitConverter.ToInt16(p, pp);
                            pp += 2;
                            float y = (float)System.BitConverter.ToInt16(p, pp);
                            pp += 2;
                            float z = (float)System.BitConverter.ToInt16(p, pp);
                            pp += 2;

                            //d($"BNO055 quaternion: {w}  {x}  {y}  {z}");

                            Quaternion q = Quaternion.identity;

                            q *= Quaternion.Euler(90, 0, 0) ;
                            q *= new Quaternion(x / 16384, y / 16384, z / 16384, w / 16384 );
                            q *= Quaternion.Euler(-90, 0, 0);

                            q *= Quaternion.Euler(0, -210 , 0);
                            q *= Quaternion.Euler(0, 180, 0);

                           
                            //Vector3 e = q.eulerAngles;
                            //d($"BNO055 q->euler: {e.x}  {e.y}  {e.z}");

                            {
                                //Atom a = SuperController.singleton.GetAtomByUid("DSBR_Chair");
                                //a.transform.rotation = q;

                            }

                            headRotation = q;

                            break;

                        }

                    case 0xff: // announce packet
                        return;

                    default: // skip unknown packet
                        {
                            int dataSize = System.BitConverter.ToInt16(p, pp);
                            pp += 2;
                            pp += dataSize - 2 - 1;
                        }
                        break;
                }

                if (pp < 0 || pp >= p.Length)
                    return;
            }
        }
        #endregion

        #region JsonStorables and the options sheet

        Dictionary<string, JSONStorableBool> jsBoolean = new Dictionary<string, JSONStorableBool>();
        Dictionary<string, JSONStorableFloat> jsFloat = new Dictionary<string, JSONStorableFloat>();

        private readonly string leftside_settings_freeform = @"

        `You need a webcam head pose sensor for this plugin. Download it from https://github.com/Eugene-E0a80fd8080ff8e/HeadTracker6DoF/tags
        ^height=120,fontSize=25

        SPACER 18.0

        `Filters are applied in following order: damper, linear predictor, deadband, limiter,  amplificator, final damper.
        ^height=90,fontSize=25

        Damper : Float [100,0,2000]
        `Value in milliseconds. (time to reach 62.3% of a new state)
        ^height=95,fontSize=25

        `Limiter
        ^height=70,fontSize=60,alignment=MiddleCenter

        Pitch limiter : Float [15,10,45]

        Yaw limiter : Float [25,10,45]

        Roll limiter : Float [25,10,45]
        `As of October 2020 HeadTracker6DoF sensor makes sudden abrupt moves on the edges of its working field. Limiter is to prevent such moves: all values beyond this bounds will be ignored.
        ^height=165,fontSize=25

        ";

        private readonly string rightside_settings_freeform = @"
        `This was not tested with VR set
        ^fontSize=30,height=40,textColor=#FF0000

        SPACER 15.0

        Dead-band angle : Float [3,0,10]
        `Camera will move only if your head have moved this far. Prevents camera shaking due to input noise. Applied before limiter.
        ^height=125,fontSize=25

        `Amplifier
        ^height=70,fontSize=60,alignment=MiddleCenter

        Pitch amplifier : Float [2,0.5,5]

        Yaw amplifier : Float [1.5,0.5,5]

        Roll amplifier : Float [-1,-3,3]

        Final damper : Float [100,0,2000]

        SPACER 15.0

        `You can use F2 button to shift middle point. Camera does not follow your head while F2 is pressed. Might be useful in POV mode. BTW, this plugin works together with Passenger and/or ImprovedPOV plugins.
        ^height=165,fontSize=25
        ";

        private void InitControls(string freeform, Boolean side)
        {
            var lines = Regex.Split(freeform, "\r\n")
                .Select(x => x.Trim())
                .Where(x => x.Count() > 0);

            UIDynamic ui = null;
            JSONStorableParam js = null;
            foreach (string line in lines)
            {
                {// a label
                    var t = Regex.Match(line, "^`(.*)$");
                    if (t.Success)
                    {
                        JSONStorableString storable = new JSONStorableString("Info", t.Groups[1].Value);
                        ui = this.CreateTextField(storable, side);
                        js = storable;
                    }
                }

                {// UIDynamic properties
                    var t = Regex.Match(line, "^\\^(.*)$");
                    if (t.Success)
                    {
                        Regex.Split(t.Groups[1].Value, ",").ToList().ForEach(s =>
                        {
                            var kv = Regex.Match(s, "^(.+)=(.*)$");
                            if (kv.Success)
                            {
                                string v = kv.Groups[2].Value;
                                switch (kv.Groups[1].Value)
                                {
                                    // modifies latest UIDynamic
                                    case "height":
                                        {
                                            float height = float.Parse(v);
                                            LayoutElement layout = ((UIDynamicTextField)ui).GetComponent<LayoutElement>();
                                            layout.preferredHeight = height;
                                            layout.minHeight = height;
                                            ui.height = height;
                                            break;
                                        }
                                    case "fontSize":
                                        {
                                            ((UIDynamicTextField)ui).UItext.fontSize = (int)(float.Parse(v));
                                            break;
                                        }
                                    case "alignment":
                                        {
                                            var a = ((UIDynamicTextField)ui).UItext;
                                            switch (v)
                                            {
                                                case "MiddleCenter": a.alignment = TextAnchor.MiddleCenter; break;
                                                case "MiddleLeft": a.alignment = TextAnchor.MiddleLeft; break;
                                                case "MiddleRight": a.alignment = TextAnchor.MiddleRight; break;
                                                case "LowerLeft": a.alignment = TextAnchor.LowerLeft; break;
                                            }
                                            break;
                                        }
                                    case "textColor":
                                        {
                                            int i = int.Parse(v.Substring(1), System.Globalization.NumberStyles.HexNumber);
                                            Color c = new Color((i >> 16) & 0xff, (i >> 8) & 0xff, i & 0xff);
                                            ((UIDynamicTextField)ui).textColor = c;
                                            break;
                                        }
                                    case "setBoolean":
                                        {
                                            ((JSONStorableBool)js).SetVal(Boolean.Parse(v));
                                            break;
                                        }
                                }
                            }
                        });
                    }
                }

                {
                    var t = Regex.Match(line, "^SPACER ([-e.0-9]+)$");
                    if (t.Success)
                    {
                        var spacer = CreateSpacer(side);
                        spacer.height = float.Parse(t.Groups[1].Value);
                    }

                }
                {
                    var t = Regex.Match(line, "^(.+) : ([^ ]+) *(\\[.*\\])?$");
                    if (t.Success)
                    {
                        string type = t.Groups[2].Value;
                        string varname = t.Groups[1].Value;
                        string titlename = t.Groups[1].Value;
                        if (titlename.Contains("/")) titlename = titlename.Substring(0, titlename.IndexOf("/"));
                        switch (type)
                        {
                            case "Boolean":
                                {
                                    jsBoolean[varname] = new JSONStorableBool(titlename, false, new JSONStorableBool.SetBoolCallback(v => BooleanSettingChanged(varname, v)));
                                    
                                    RegisterBool(jsBoolean[varname]);
                                    CreateToggle(jsBoolean[varname], side);

                                    BooleanSettingChanged(varname, jsBoolean[varname].val);

                                    js = jsBoolean[varname];
                                    break;
                                }
                            case "Float":
                                {
                                    var tt = Regex.Match(t.Groups[3].Value, "^\\[([-.0-9]+),([-.0-9]+),([-.0-9]+)\\]$");
                                    jsFloat[varname] = new JSONStorableFloat(
                                        titlename
                                        , float.Parse(tt.Groups[1].Value)
                                        , new JSONStorableFloat.SetFloatCallback(v => FloatSettingChanged(varname, v))
                                        , float.Parse(tt.Groups[2].Value)
                                        , float.Parse(tt.Groups[3].Value)
                                        , true
                                    );

                                    RegisterFloat(jsFloat[varname]);
                                    CreateSlider(jsFloat[varname], side);

                                    FloatSettingChanged(varname, jsFloat[varname].val);

                                    js = jsFloat[varname];
                                    break;
                                }
                        }
                    }

                }
            }
        }

        private void BooleanSettingChanged(string varname, bool value)
        {

        }

        private void FloatSettingChanged(string varname, float value)
        {
            d($" {varname} = {value} ");
            if (varname == "Dead-band angle") deadbandAngle = value;
            else if (varname == "Damper") damper_ms = value;
            else if (varname == "Final damper") finalDamper_ms = value;

            else if (varname == "Pitch limiter") limiter_x = value;
            else if (varname == "Yaw limiter") limiter_y = value;
            else if (varname == "Roll limiter") limiter_z = value;

            else if (varname == "Pitch amplifier") amplification_x = value;
            else if (varname == "Yaw amplifier") amplification_y = value;
            else if (varname == "Roll amplifier") amplification_z = value;

            else
            {
                d($"Unknown variable : {varname}");
            }

        }

        #endregion

        #region MVR plugin lifecycle entries

        public override void Init()
        {
            try
            {
                InitControls(leftside_settings_freeform, false);
                InitControls(rightside_settings_freeform, true);

                fUiLastActive = Environment.TickCount;
            }
            catch (Exception e)
            {
                SuperController.LogError("Exception caught: " + e);
            }
        }

        // Start is called once before Update or FixedUpdate is called and after Init()
        void Start()
        {
            try
            {
            }
            catch (Exception e)
            {
                SuperController.LogError("Exception caught: " + e);
            }
        }


        bool F2_pressed;
        //Quaternion prevRot = Quaternion.identity;
        Quaternion baseRot = Quaternion.identity;
        Quaternion relRot = Quaternion.identity;

        // Update is called with each rendered frame by Unity
        void Update()
        {
            fUiLastActive = Environment.TickCount;

            //d($" terminateReceivingThread = {terminateReceivingThread}");
            //d($" thread = {thread}");

            if (null == thread)
            {
                d($"staring the thread for UDP listener");
                thread = new Thread(new ThreadStart(TrFunc));
                thread.Start();
            }

            try
            {
                Queue queue;
                lock (dataChannel.SyncRoot)
                {
                    queue = (Queue)(dataChannel.Clone());
                    dataChannel.Clear();
                }
                while (queue.Count > 0)
                {
                    var data = (TimedData)queue.Dequeue();
                    processPacket(data.data, data.time);
                }

                Quaternion rot = calc();

                SuperController.singleton.MonitorRig.rotation = SuperController.singleton.MonitorRig.rotation * Quaternion.Inverse(baseRot * relRot);

                if (Input.GetKeyDown(KeyCode.F2))
                {
                    F2_pressed = true;
                    Quaternion newBaseRot = relRot;
                    Quaternion newRelRot = Quaternion.Inverse(relRot) * baseRot * relRot;
                    baseRot = newBaseRot;
                    relRot = newRelRot;

                }
                if (Input.GetKeyUp(KeyCode.F2))
                {
                    F2_pressed = false;
                    Quaternion newBaseRot = baseRot  * relRot * Quaternion.Inverse(baseRot) ;
                    Quaternion newRelRot = baseRot;
                    baseRot = newBaseRot;
                    relRot = newRelRot;
                }

                if (F2_pressed)
                {
                    Quaternion newBaseRot = rot;
                    Quaternion newRelRot = Quaternion.Inverse(rot) * baseRot * relRot;
                    baseRot = newBaseRot;
                    relRot = newRelRot;

                    damper = null;
                    deadband = null;
                    finalDamper = null;
                    linearPredictor = null;
                }
                else relRot = rot;

                SuperController.singleton.MonitorRig.rotation = SuperController.singleton.MonitorRig.rotation * baseRot * relRot;

                //prevRot = baseRot * relRot;

                /*SuperController.singleton.MonitorRig.rotation = SuperController.singleton.MonitorRig.rotation * Quaternion.Inverse(prevRot);
                SuperController.singleton.MonitorRig.rotation = SuperController.singleton.MonitorRig.rotation * rot;
                prevRot = rot;
                */

            }
            catch (Exception e)
            {
                SuperController.LogError("Exception caught: " + e);
            }

            fUiLastActive = Environment.TickCount;
        }


        // FixedUpdate is called with each physics simulation frame by Unity
        void FixedUpdate()
        {
            try
            {
                fUiLastActive = Environment.TickCount;
            }
            catch (Exception e)
            {
                SuperController.LogError("Exception caught: " + e);
            }
        }


        // OnDestroy is where you should put any cleanup
        // if you registered objects to supercontroller or atom, you should unregister them here
        void OnDestroy()
        {
            //SuperController.singleton.navigationRig.rotation = Quaternion.Inverse(prevRot) * SuperController.singleton.navigationRig.rotation;
            SuperController.singleton.navigationRig.rotation = SuperController.singleton.navigationRig.rotation * Quaternion.Inverse( baseRot * relRot );

            if (null != terminateReceivingThread)
                terminateReceivingThread();

        }

        #endregion

        #region A thread for listening UDP socket

        private readonly Queue dataChannel = new Queue();
        private volatile Thread thread;
        private volatile Action terminateReceivingThread = null;
        private volatile int fUiLastActive = 0;

        struct TimedData
        {
            public byte[] data;
            public int time;
        }

        public void TrFunc()
        {
            d($"TrFunc started");
            UdpClient udpClient = null;

            this.terminateReceivingThread = () =>
            {
                d("*** terminateReceivingThread");

                if (udpClient != null)
                {
                    udpClient.Close();
                    udpClient = null;
                }
                Thread toTerminate = thread;
                thread = null;
                toTerminate.Join();
                this.terminateReceivingThread = null;
            };

            int c = 0;
            do
            {
                try
                {
                    udpClient = new UdpClient(62731);
                }
                catch (Exception)
                {
                    Thread.Sleep(100);
                    d($"reconnecting to UDP : {++c}");
                }
            } while (udpClient == null && thread == Thread.CurrentThread);
            d($"connected to UDP : {c}");

            if (udpClient == null || thread != Thread.CurrentThread)
            {
                if (null != terminateReceivingThread)
                    this.terminateReceivingThread();
            }


            IPEndPoint remoteIP = new IPEndPoint(IPAddress.Any, 0);
            IPEndPoint broadcastIP = new IPEndPoint(IPAddress.Broadcast, 62731);

            udpClient.Client.ReceiveTimeout = 100; //ms

            while (thread == Thread.CurrentThread)
            {
                Byte[] data = null;
                try
                {
                    data = udpClient.Receive(ref remoteIP);
                }
                catch (SocketException)
                { // probably timeout 
                }

                int t = Environment.TickCount;

                if (null != data)
                {
                    var td = new TimedData();
                    td.data = data;
                    td.time = t;

                    lock (dataChannel.SyncRoot)
                    { // cleanup
                        if (dataChannel.Count >= 5) dataChannel.Dequeue();
                        if (dataChannel.Count >= 5) dataChannel.Dequeue();
                        dataChannel.Enqueue(td);
                    }
                }

                if (fUiLastActive + 1000 < Environment.TickCount)
                {
                    // need to terminate if got disconneected from UI thread events (i.e. plugin unloaded)
                    d($"UI timeout");
                    if (null != terminateReceivingThread)
                        this.terminateReceivingThread();
                    break;
                }

            }
        }
        #endregion

        #region UDP messages for debugging

        // this is just for debuging. Not related to other UDP activities
        // should be disabled upon release
        private UdpClient __d_udpClient = null;
        void d(string str)
        {
            
            if (null == __d_udpClient)
            {
                try
                {
                    __d_udpClient = new UdpClient();
                }
                catch (Exception e)
                {
                }
            }
            IPEndPoint remoteIP = new IPEndPoint(IPAddress.Parse("192.168.1.11"), 65533);
            byte[] b = ASCIIEncoding.ASCII.GetBytes(str);

            __d_udpClient.Send(b, b.Length, remoteIP);
            
            // tcpdump -Aqnn udp port 65533
            // or, nicer:
            // unbuffer tcpdump -Aqnn udp port 65533 | stdbuf -o0 grep -v '192.168' | cut -b 29-
        }
        #endregion
    }
}