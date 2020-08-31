using System;
using Xamarin.Forms;
using Xamarin.Forms.Xaml;

namespace fire_impact
{
    public partial class App : Application
    {
        // I'm putting these here because I can't access them anywhere else
        public short[] sensorData = {0, 0};
        public bool connected = false;

        public App()
        {
            InitializeComponent();

            MainPage = new AppShell();
        }

        protected override void OnStart()
        {
        }

        protected override void OnSleep()
        {
        }

        protected override void OnResume()
        {
        }
    }
}
