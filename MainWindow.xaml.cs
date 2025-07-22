using MyModbusRtu;
 using System.Text;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Data;
using System.Windows.Documents;
using System.Windows.Input;
using System.Windows.Media;
using System.Windows.Media.Imaging;
using System.Windows.Navigation;
using System.Windows.Shapes;

namespace Master2
{
    /// <summary>
    /// Interaction logic for MainWindow.xaml
    /// </summary>
    public partial class MainWindow : Window
    {
        private string? selected_port = null;
        private string? selected_led = null;
        private System.IO.Ports.SerialPort? port;

        private string[] LEDS = {"LED 1", "LED 2", "LED 3", "LED 4"};

        private StateLEDS state_leds = new StateLEDS();

        private readonly Dictionary<int, ushort[]> led_masks = new Dictionary<int, ushort[]>
        {
            {1, new ushort[] { 0x0000, 0x2000 } },
            {2, new ushort[] { 0x0000, 0x4000 } },
            {3, new ushort[] { 0x0008, 0x0000 } },
            {4, new ushort[] { 0x0010, 0x0000 } }
        };


        public MainWindow()
        {
            InitializeComponent();

            string[] ports = System.IO.Ports.SerialPort.GetPortNames();
            SelectorComPort.ItemsSource = ports;
            SelectorLED.ItemsSource = LEDS;

        }

        private void SelectorComPort_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            object item = SelectorComPort.SelectedItem;

            if (item != null)
            {
                selected_port = (string)item;

                MessageBox.Show($"{selected_port} was selected.");
            }
        }

        private void SelectorLED_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            object item = SelectorLED.SelectedItem;
            if (item != null)
            {
                selected_led = (string)item;
            }
        }

        private void TurnOnLED_Click(object sender, RoutedEventArgs e)
        {
            if (selected_port == null || selected_led == null)
            {
                MessageBox.Show("COM-port or LED were not selected.");
                return;
            }

            int ledNumber = int.Parse(selected_led.Replace("LED ", ""));

            ModbusState state = new ModbusState
            {
                SelectedPortName = selected_port,
                SlaveAddress = 0x01
            };

            ushort register_address = 0x0000;

            byte[] read_request = ModbusProtocol.BuildReadRequest(state.SlaveAddress, register_address, 2);
            ModbusProtocol.SendAndReceive(state, read_request);

            if (state.IsError || state.LastResponse == null)
            {
                MessageBox.Show("Error: " + (state.ErrorCode ?? "No response received"));
                return;
            }

            ushort[] gpio_value = new ushort[2];
            gpio_value[0] = (ushort)((state.LastResponse[3] << 8) | state.LastResponse[4]); // GPIOA
            gpio_value[1] = (ushort)((state.LastResponse[5] << 8) | state.LastResponse[6]); // GPIOC

            ushort[] mask = led_masks[ledNumber];

            gpio_value[0] |= mask[0];
            gpio_value[1] |= mask[1];

            byte[] write_request = ModbusProtocol.BuildWriteRequest(state.SlaveAddress, register_address, gpio_value);
            ModbusProtocol.SendAndReceive(state, write_request);

            if (state.IsError)
            {
                MessageBox.Show("Error writing: " + (state.ErrorCode ?? "No response"));
                return;
            }

            UpdateLedStatesFromGpio(gpio_value);

        }

        private void TurnOffLED_Click(object sender, RoutedEventArgs e)
        {
            if (selected_port == null || selected_led == null)
            {
                MessageBox.Show("COM-port or LED were not selected.");
                return;
            }

            int ledNumber = int.Parse(selected_led.Replace("LED ", ""));

            ModbusState state = new ModbusState
            {
                SelectedPortName = selected_port,
                SlaveAddress = 0x01
            };

            ushort register_address = 0x0000;

            byte[] read_request = ModbusProtocol.BuildReadRequest(state.SlaveAddress, register_address, 2);
            ModbusProtocol.SendAndReceive(state, read_request);

            if (state.IsError || state.LastResponse == null || state.LastResponse.Length < 7)
            {
                MessageBox.Show("Error: " + (state.ErrorCode ?? "No response received"));
                return;
            }

            ushort[] gpio_value = new ushort[2];
            gpio_value[0] = (ushort)((state.LastResponse[3] << 8) | state.LastResponse[4]); // GPIOA
            gpio_value[1] = (ushort)((state.LastResponse[5] << 8) | state.LastResponse[6]); // GPIOC

            ushort[] mask = led_masks[ledNumber];

            gpio_value[0] &= (ushort)~mask[0];
            gpio_value[1] &= (ushort)~mask[1];

            byte[] write_request = ModbusProtocol.BuildWriteRequest(state.SlaveAddress, register_address, gpio_value);
            ModbusProtocol.SendAndReceive(state, write_request);

            if (state.IsError)
            {
                MessageBox.Show("Error writing: " + (state.ErrorCode ?? "No response"));
                return;
            }

            UpdateLedStatesFromGpio(gpio_value);

        }


        private void UpdateState_Click(object sender, RoutedEventArgs e)
        {
            if (selected_port == null)
            {
                MessageBox.Show("COM-port not selected.");
                return;
            }

            ModbusState state = new ModbusState
            {
                SelectedPortName = selected_port,
                SlaveAddress = 0x01
            };

            ushort register_address = 0x0000;

            byte[] read_request = ModbusProtocol.BuildReadRequest(state.SlaveAddress, register_address, 2);
            ModbusProtocol.SendAndReceive(state, read_request);

            if (state.IsError || state.LastResponse == null)
            {
                MessageBox.Show("Error: " + (state.ErrorCode ?? "no answer came"));
                return;
            }

            ushort[] gpio_value = new ushort[2];

            gpio_value[0] = (ushort)((state.LastResponse[3] << 8) | state.LastResponse[4]); // GPIOA
            gpio_value[1] = (ushort)((state.LastResponse[5] << 8) | state.LastResponse[6]); // GPIOC


            UpdateLedStatesFromGpio(gpio_value);
        }


        private void UpdateLedStatesFromGpio(ushort[] gpio_value)
        {
            state_leds.Led1 = (gpio_value[1] & led_masks[1][1]) != 0;
            state_leds.Led2 = (gpio_value[1] & led_masks[2][1]) != 0;
            state_leds.Led3 = (gpio_value[0] & led_masks[3][0]) != 0;
            state_leds.Led4 = (gpio_value[0] & led_masks[4][0]) != 0;

            UpdateVisualLeds();
        }

        private void UpdateVisualLeds()
        {
            LED1.Fill = state_leds.Led1 ? Brushes.Green : Brushes.White;
            LED2.Fill = state_leds.Led2 ? Brushes.Green : Brushes.White;
            LED3.Fill = state_leds.Led3 ? Brushes.Green : Brushes.White;
            LED4.Fill = state_leds.Led4 ? Brushes.Green : Brushes.White;
        }

        private void Send_Click(object sender, RoutedEventArgs e)
        {
            if (selected_port == null)
            {
                MessageBox.Show("COM-port was not selected.");
                return;
            }
            

            byte[] bytes = HexStringToBytes(Request.Text);

            ModbusState state = new ModbusState
            {
                SelectedPortName = selected_port,
                SlaveAddress = bytes[0]
            };

            bytes = ModbusProtocol.AppendCRC(bytes);

            ModbusProtocol.SendAndReceive(state, bytes);

            if (state.IsError || state.LastResponse == null)
            {
                MessageBox.Show("Error: " + (state.ErrorCode ?? "no answer came"));
                return;
            }

            string response = BitConverter.ToString(state.LastResponse).Replace("-", "");

            Response.Text = response;

        }

        private byte[] HexStringToBytes(string hex)
        {
            hex = hex.Replace(" ", "").ToUpper();

            if (hex.Length % 2 != 0)
            {
                hex = "0" + hex;
            }

            int byteCount = hex.Length / 2;
            byte[] result = new byte[byteCount];

            string byteValue;

            for (int i = 0; i < byteCount; i++)
            {
                byteValue = hex.Substring(i * 2, 2);
                result[i] = Convert.ToByte(byteValue, 16);
            }

            return result;
        }

    }

    public class StateLEDS
    {
        public bool Led1 { get; set; }
        public bool Led2 { get; set; }
        public bool Led3 { get; set; }
        public bool Led4 { get; set; }

    }


}