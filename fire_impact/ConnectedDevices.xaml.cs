﻿using System;
using System.Net;
using System.Net.Sockets;
using System.Text;
using System.ComponentModel;
using Xamarin.Forms;
using System.Threading.Tasks;
using System.Timers;

namespace fire_impact
{
    // Learn more about making custom code visible in the Xamarin.Forms previewer
    // by visiting https://aka.ms/xamarinforms-previewer
    [DesignTimeVisible(false)]
    public partial class ConnectedDevices : ContentPage
    {
        private enum BroadcastResult
        {
            NetworkUnreachable,
            Success,
            TimedOut
        }

        public ConnectedDevices()
        {
            // Start the page
            InitializeComponent();

            // Start scanning
            ScanForDevices();
        }

        private void CreateRetryButton()
        {
            // Create a retry button that will rescan for the arduino when it's clicked
            Button retryButton = new Button()
            {
                Text = "Retry",
                BackgroundColor = Color.Blue,
                TextColor = Color.White
            };

            retryButton.Clicked += delegate (object sender, EventArgs e)
            {
                // Reset the ping count text
                pingCountLabel.Text = "";
                // Delete the retry button
                stackLayout.Children.Remove((Button)sender);

                // Start scanning
                ScanForDevices();
            };

            // Add the button to the stack layout
            stackLayout.Children.Add(retryButton);
        }

        /// <summary>
        /// This method will call the broadcast method, and then decide whether we have got any valid repsonses. It will keep broadcasting until we find an arduino.
        /// </summary>
        private async void ScanForDevices()
        {
            // We can jump back to here when we need to rescan for arduinos. This stop us having to do recursion and then filling up lots of memory
            top_of_function:

            // We have started sending
            statusLabel.Text = "Sending...";

            // Create the socket. We will use this to bradcast the discovery packet
            Socket socket = new Socket(AddressFamily.InterNetwork, SocketType.Dgram, ProtocolType.Udp)
            {
                // If this isn't set then we get an access denied exception
                EnableBroadcast = true
            };

            // Broadcast the packet and get the result
            var tuple = await Broadcast(socket);
            BroadcastResult status = tuple.Item1;
            UdpReceiveResult response = tuple.Item2;

            // Holds the amount of times we have pinged the network
            byte pingCount = 1;
            pingCountLabel.Text = "1 ping";

            // If we got a network unreachable then the user isn't connected
            if(status == BroadcastResult.NetworkUnreachable)
            {
                CreateRetryButton();
                statusLabel.Text = "Couldn't find any devices because the network is unreachable. Please connect to a WiFi network or hotspot and try again. Note: using mobile data will not work.";
                // We dont need to rescan so we can exit the function
                return;
            }

            // If it timed out we need to ping the network again.
            while (status == BroadcastResult.TimedOut)
            {
                tuple = await Broadcast(socket);
                status = tuple.Item1;
                response = tuple.Item2;

                pingCount++;
                pingCountLabel.Text = $"{pingCount} pings";

                // If we have pinged 3 times then we really can't find the arduino so we gotta stop
                if (pingCount == 3)
                {
                    statusLabel.Text = "No devices found";

                    CreateRetryButton();
                    // We dont need to rescan so we can exit the function
                    return;
                }
            }

            // Otherwise, we have received the data. Notify the app.
            statusLabel.Text = $"Connected to the arduino";

            // Start the tcp loop as a task. This will run it in the background withput interupting/blocking the main UI thread
            await Task.Run(() => {
                // When the function exits it will be because of a connection failure.
                TCPLoop(response.RemoteEndPoint.Address.ToString());
                // Give the arduino a bit of time to sort its problems out
                Task.Delay(2000);
            });

            // We want the function to rescan for any arduinos
            goto top_of_function;
        }

        private void TCPLoop(string hostName)
        {
            TcpClient TCPClient = null;
            // If this gets set to false, then we have lost the connection and we want to exit the tcp handler loop
            bool breakCheck = true;

            // Create a timer. This runs an event after 5000ms have passed. If we haven't received something from the arduino in that time, something has gone wrong.
            Timer timer = new Timer(5000)
            {
                // We dont want it to start right now: bcwbkc
                Enabled = false
            };
            // If the elapsed event is fired, we want to close the listener because we haven't got a response
            timer.Elapsed += delegate (object sender, ElapsedEventArgs e)
            {
                // If this method has been triggered, then we have lost connection with the arduino. We need to exit the loop and return out of the function
                breakCheck = false;
            };

            // Create the tcp client
            TCPClient = new TcpClient(hostName, 1305);
            // This will be the network stream that we use to receive the data from the arduino
            NetworkStream networkStream = TCPClient.GetStream();

            // Send the initial piece of data so the arduino can store this client
            networkStream.Write(Encoding.ASCII.GetBytes("init"), 0, 4);

            // We are now connected, so the sensor data page should start displaying results
            ((App)Application.Current).connected = true;

            // Start the timer
            timer.Start();

            // Loop forever. We can break out of it if we get an exception
            while (breakCheck == true)
            {
                // If there is no data available to read from the arduino then we don't want to read it
                if(networkStream.DataAvailable == false) continue;

                // Buffer to store the data
                byte[] data = new byte[16];

                // Read in the data
                networkStream.Read(data, 0, data.Length);

                // We have received some data so we can restart the timer
                timer.Stop();
                timer.Start();

                // Turn the data into a number that we can use
                string[] sensorData = Encoding.ASCII.GetString(data).Split(',');

                for(byte i = 0; i < 2; i++)
                {
                    short sensorDataNum = short.Parse(sensorData[i]);

                    // Put the number in the main app's array so that the sensor data content-page can access it
                    ((App)Application.Current).sensorData[i] = sensorDataNum;
                }
            }

            // We aren't connected anymore, let the sensor data screen know
            ((App)Application.Current).connected = false;

            // Release the timer and tcp client's resources. We can then exit the function
            timer.Dispose();
            TCPClient.Close();
        }


        /// <summary>
        /// Sends out a UDP broadcast and listens for the result. If it doesnt get a response in 5 seconds, it times out and returns a blank response.
        /// </summary>
        private async Task<(BroadcastResult, UdpReceiveResult)> Broadcast(Socket socket)
        {
            // Parse out the address from 255.255.255.255 - this address will make the socket send to everyone
            IPAddress broadcastIP = IPAddress.Parse("255.255.255.255");

            try
            {
                // Send the encoded string on port 1304 with the broadcast IP
                socket.SendTo(Encoding.ASCII.GetBytes("areYouTheArduino"), new IPEndPoint(broadcastIP, 1304));
            }
            // If the user isn't connected to the network then SendTo throws an error
            catch(SocketException)
            {
                // We want to return an empty receive result
                return (BroadcastResult.NetworkUnreachable, new UdpReceiveResult());
            }

            // Now we need to create a listener to listen for any response packets
            UdpClient listener = new UdpClient(1304);
            // This will hold the result if there is one, otherwise if the receiver times out its buffer will be null
            UdpReceiveResult receiveResult;

            // Create a timer. This runs an event after 5000ms have passed. We can then stop listening since we haven't got a response
            Timer timer = new Timer(5000)
            {
                // We only want it to run once
                AutoReset = false,
                Enabled = false
            };

            // If the elapsed event is fired, we want to close the listener because we haven't got a response
            timer.Elapsed += delegate(object sender, ElapsedEventArgs e) { listener.Close(); };

            // wait to receive the data. We need to try-catch this because when we stop the listener with the above delegate,
            // it will be a disposed object and it'll throw an exception.
            try {
                while(true)
                {
                    // Start the timer
                    timer.Start();
                    receiveResult = await listener.ReceiveAsync();

                    // We need to check that it was the arduino replying and not just some rando packet
                    if (Encoding.ASCII.GetString(receiveResult.Buffer) == "I am an arduino")
                    {
                        // If it was the arduino, then we can exit this while loop and return back to ScanForDevices()
                        timer.Stop();
                        break;
                    }
                }
            }
            catch(ObjectDisposedException)
            {
                // Return the empty result if we didn't get a response
                return (BroadcastResult.TimedOut, receiveResult);
            }

             // Delete the timer
            timer.Dispose();
            // Release the listener
            listener.Close();

            // If we did get a response, then we can pass it back
            return (BroadcastResult.Success, receiveResult);
        }
    }
}