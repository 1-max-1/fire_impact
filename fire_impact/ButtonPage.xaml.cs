using System;
using Xamarin.Forms;
using Xamarin.Forms.Xaml;

namespace fire_impact
{
    [XamlCompilation(XamlCompilationOptions.Compile)]
    public partial class ButtonPage : ContentPage
    {
        int clickCount = 0;

        public ButtonPage()
        {
            InitializeComponent();
        }

        void OnButtonClick(object sender, EventArgs args)
        {
            clickCount++;
            button.Text = $"I have been clicked {clickCount} times";
        }
    }
}