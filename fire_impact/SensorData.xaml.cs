using Xamarin.Forms;
using Xamarin.Forms.Xaml;
using SkiaSharp;
using SkiaSharp.Views.Forms;
using System;
using System.Threading.Tasks;
using System.Timers;

namespace fire_impact
{
    [XamlCompilation(XamlCompilationOptions.Compile)]
    public partial class SensorData : ContentPage
    {
        //private int sensorNum = 200;
        private readonly SKCanvasView canvasView;

        public SensorData()
        {
            InitializeComponent();

            // Create a new skia sharp canvas and asign the drawing method
            canvasView = new SKCanvasView();
            canvasView.PaintSurface += OnCanvasViewPaintSurface;

            // Add the canvas to the page content
            Content = canvasView;

            // Start running the draw loop as a Task so it doesn't bock the UI thread
            Task.Run(() => { DrawLoop(); });            
        }

        private void DrawLoop()
        {
            // Create a timer. This runs an event every 17ms, effectivley creating a draw loop of approximately 60 fps (technically 58.824 fps).
            Timer timer = new Timer(17)
            {
                // We want it to run continously
                AutoReset = true,
                // Start it from timer.Start(); instead of in here because we be kool kids
                Enabled = false
            };

            // If the elapsed event is fired, we want to execute the drawing code
            timer.Elapsed += delegate (object sender, ElapsedEventArgs e)
            {
                // Trigger the redrawing of the canvas view
                canvasView.InvalidateSurface();
            };

            timer.Start();
        }

        void OnCanvasViewPaintSurface(object sender, SKPaintSurfaceEventArgs args)
        {
            // Get the canvas for drawing on
            SKCanvas canvas = args.Surface.Canvas;
            float halfWidth = args.Info.Width / 2;
            float halfHeight = args.Info.Height / 2;

            // Clear the canvas of any previous drawings
            canvas.Clear();

            // If there is no device connected then we dont want to display the sensor data
            if (((App)Application.Current).connected == false)
            {
                canvas.DrawText("Connect to an arduino first", halfWidth, halfHeight, new SKPaint
                {
                    Style = SKPaintStyle.Fill,
                    Color = Color.Red.ToSKColor(),
                    TextSize = 80,
                    TextAlign = SKTextAlign.Center
                });
                return;
            }
            else
            {
                canvas.DrawText("Light level", halfWidth, halfHeight, new SKPaint
                {
                    Style = SKPaintStyle.Fill,
                    Color = Color.Black.ToSKColor(),
                    TextSize = 80,
                    TextAlign = SKTextAlign.Center
                });
            }

            Color[] gaugeColors = { Color.Blue, Color.LawnGreen};

            for(byte i = 0; i < 2; i++)
            {
                DrawGauge(halfWidth, halfHeight + (800 * i - 400), gaugeColors[i], ((App)Application.Current).sensorData[i], canvas);
            }
        }

        // Increases a bounding box rectangle by a certain radius
        void InflateRect(ref SKRect rect, int radius)
        {
            rect.Left -= radius;
            rect.Right += radius;
            rect.Top -= radius;
            rect.Bottom += radius;
        }

        // Decreases a bounding box rect by a certain amount
        void DeflateRect(ref SKRect rect, int radius)
        {
            rect.Left += radius;
            rect.Right -= radius;
            rect.Top += radius;
            rect.Bottom -= radius;
        }

        // Draws a gauge at the specified coordinates with the specified color
        void DrawGauge(float x, float y, Color color, short sensorData, SKCanvas canvas)
        {
            // Bounding box for containing the arcs
            SKRect boundingBox = new SKRect(x - 200, y - 200, x + 200, y + 200);

            // This patch will hold the outline arcs for the meter
            SKPath outlinePath = new SKPath();
            // This one holds the arc that actually changes based on the sensors
            SKPath gaugePath = new SKPath();

            // Add the first outline arc
            outlinePath.AddArc(boundingBox, 135, 270);

            // Decrease the bounding box because the other outline arc is smaller
            DeflateRect(ref boundingBox, 23);
            outlinePath.AddArc(boundingBox, 135, 270);

            // Increase the bounding box for the main arc
            InflateRect(ref boundingBox, 12);
            // Add the main arc based on the data from the sensor
            gaugePath.AddArc(boundingBox, 135, 270 * sensorData / 4095);

            SKPaint outlineArcPaint = new SKPaint
            {
                Style = SKPaintStyle.Stroke,
                StrokeCap = SKStrokeCap.Butt,
                Color = Color.SlateGray.ToSKColor(),
                StrokeWidth = 3
            };

            // Draw the outline arcs
            canvas.DrawPath(outlinePath, outlineArcPaint);

            // Draw the outline cap lines
            canvas.DrawLine(x - (float)Math.Cos(Math.PI / 4) * 177, y + (float)Math.Sin(Math.PI / 4) * 177, x - (float)Math.Cos(Math.PI / 4) * 200, y + (float)Math.Sin(Math.PI / 4) * 200, outlineArcPaint);
            canvas.DrawLine(x + (float)Math.Cos(Math.PI / 4) * 177, y + (float)Math.Sin(Math.PI / 4) * 177, x + (float)Math.Cos(Math.PI / 4) * 200, y + (float)Math.Sin(Math.PI / 4) * 200, outlineArcPaint);

            // Draw the sensor data gauge
            canvas.DrawPath(gaugePath, new SKPaint
            {
                Style = SKPaintStyle.Stroke,
                StrokeCap = SKStrokeCap.Butt,
                Color = color.ToSKColor(),
                StrokeWidth = 21
            });

            // Draw the text based on the sensor data
            canvas.DrawText($"{Math.Round((float)(100 * sensorData / 4095))}%", x, y, new SKPaint
            {
                Style = SKPaintStyle.Fill,
                Color = Color.Black.ToSKColor(),
                TextSize = 70,
                TextAlign = SKTextAlign.Center
            });
        }
    }
}