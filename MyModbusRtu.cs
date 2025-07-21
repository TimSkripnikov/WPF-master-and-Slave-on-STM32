using System;
using System.Collections.Generic;
using System.IO;
using System.IO.Ports;

namespace MyModbusRtu
{
    public class ModbusState
    {
        public string? SelectedPortName { get; set; }
        public byte[]? LastRequest { get; set; }
        public byte[]? LastResponse { get; set; }
        public byte SlaveAddress { get; set; }
        public bool IsError { get; set; }
        public string? ErrorCode { get; set; }
        public bool RequestInProgress { get; set; }
    }

    public static class ModbusProtocol
    {
        // Function 0x03
        public static byte[] BuildReadRequest(byte slaveAddress, ushort startAddress, ushort numRegisters)
        {
            byte[] request = new byte[6];
            request[0] = slaveAddress;
            request[1] = 0x03;
            request[2] = (byte)(startAddress >> 8);
            request[3] = (byte)(startAddress);
            request[4] = (byte)(numRegisters >> 8);
            request[5] = (byte)(numRegisters);

            request = AppendCRC(request);
            return request;
        }

        // Function 0x10
        public static byte[] BuildWriteRequest(byte slaveAddress, ushort startAddress, ushort[] values)
        {
            int numRegisters = values.Length;
            int byteCount = numRegisters * 2;
            byte[] request = new byte[7 + byteCount];
            request[0] = slaveAddress;
            request[1] = 0x10;
            request[2] = (byte)(startAddress >> 8);
            request[3] = (byte)(startAddress);
            request[4] = (byte)(numRegisters >> 8);
            request[5] = (byte)(numRegisters);
            request[6] = (byte)byteCount;

            for (int i = 0; i < numRegisters; i++)
            {
                request[7 + i * 2] = (byte)(values[i] >> 8);
                request[7 + i * 2 + 1] = (byte)(values[i]);
            }

            request = AppendCRC(request);
            return request;
        }

        public static byte[] AppendCRC(byte[] request)
        {
            int length = request.Length;
            byte[] result = new byte[length + 2];
            Array.Copy(request, result, length);
            ushort crc = CRC16.CalculateCRC(request, length);
            result[length] = (byte)(crc & 0xFF);
            result[length + 1] = (byte)((crc >> 8) & 0xFF);
            return result;
        }

        public static void SendAndReceive(ModbusState state, byte[] request, int timeout = 2000)
        {
            if (string.IsNullOrEmpty(state.SelectedPortName))
                throw new InvalidOperationException("COM port is not selected.");

            if (state.RequestInProgress)
                throw new InvalidOperationException("Previous request is still being processed.");

            state.RequestInProgress = true;
            state.IsError = false;
            state.ErrorCode = null;

            state.LastRequest = request;

            using (SerialPort serialPort = new SerialPort(state.SelectedPortName, 9600, Parity.None, 8, StopBits.One))
            {
                serialPort.ReadTimeout = timeout;
                serialPort.WriteTimeout = timeout;

                try
                {
                    serialPort.Open();
                    serialPort.DiscardInBuffer();
                    serialPort.DiscardOutBuffer();

                    serialPort.Write(request, 0, request.Length);

                    List<byte> response = new List<byte>();

                    response.Add((byte)serialPort.ReadByte());

                    byte function = (byte)serialPort.ReadByte();
                    response.Add(function);

                    if ((function & 0x80) != 0)
                    {
                        response.Add((byte)serialPort.ReadByte());
                        response.Add((byte)serialPort.ReadByte());
                        response.Add((byte)serialPort.ReadByte());
                    }
                    else if (function == 0x03)
                    {
                        byte byteCount = (byte)serialPort.ReadByte();
                        response.Add(byteCount);

                        for (int i = 0; i < byteCount; i++)
                        {
                            response.Add((byte)serialPort.ReadByte());
                        }

                        response.Add((byte)serialPort.ReadByte());
                        response.Add((byte)serialPort.ReadByte());
                    }
                    else if (function == 0x10)
                    {
                        for (int i = 0; i < 4; i++)
                        {
                            response.Add((byte)serialPort.ReadByte());
                        }

                        response.Add((byte)serialPort.ReadByte());
                        response.Add((byte)serialPort.ReadByte());
                    }
                    else
                    {
                        throw new InvalidOperationException($"Unknown Modbus function: 0x{function:X2}");
                    }

                    byte[] responseArray = response.ToArray();

                    byte[] responseWithoutCRC = CheckCRCAndStrip(responseArray);

                    if (responseWithoutCRC.Length < 2 || responseWithoutCRC[0] != state.SlaveAddress)
                        throw new InvalidDataException($"Response from unexpected device. Expected address {state.SlaveAddress}, got {responseWithoutCRC[0]}");

                    state.LastResponse = responseWithoutCRC;

                    bool isError = (responseWithoutCRC[1] & 0x80) != 0;
                    state.IsError = isError;

                    if (isError)
                    {
                        if (responseWithoutCRC.Length < 3)
                            throw new InvalidDataException("Modbus error: response too short to contain error code.");

                        byte errorCode = responseWithoutCRC[2];
                        state.ErrorCode = $"Modbus error code: 0x{errorCode:X2}";
                    }
                    else
                    {
                        state.ErrorCode = null;
                    }
                }
                catch (TimeoutException)
                {
                    state.IsError = true;
                    state.ErrorCode = "Timeout: no response received.";
                    state.LastResponse = null;
                }
                catch (Exception ex)
                {
                    state.IsError = true;
                    state.ErrorCode = $"Error: {ex.Message}";
                    state.LastResponse = null;
                }
                finally
                {
                    if (serialPort.IsOpen)
                        serialPort.Close();

                    state.RequestInProgress = false;
                }
            }
        }

        public static byte[] CheckCRCAndStrip(byte[] response)
        {
            if (response.Length < 4)
                throw new InvalidDataException("Response too short to contain CRC.");

            int lengthWithoutCRC = response.Length - 2;

            ushort calculatedCRC = CRC16.CalculateCRC(response, lengthWithoutCRC);
            ushort receivedCRC = (ushort)((response[response.Length - 1] << 8) | response[response.Length - 2]);

            if (calculatedCRC != receivedCRC)
                throw new InvalidDataException($"CRC mismatch. Expected 0x{calculatedCRC:X4}, received 0x{receivedCRC:X4}");

            byte[] responseWithoutCRC = new byte[lengthWithoutCRC];
            Array.Copy(response, 0, responseWithoutCRC, 0, lengthWithoutCRC);
            return responseWithoutCRC;
        }
    }

    public static class CRC16
    {
        public static ushort CalculateCRC(byte[] data, int length)
        {
            ushort crc = 0xFFFF;
            for (int index = 0; index < length; index++)
            {
                crc ^= data[index];
                for (int i = 0; i < 8; i++)
                {
                    if ((crc & 0x0001) != 0)
                    {
                        crc = (ushort)((crc >> 1) ^ 0xA001);
                    }
                    else
                    {
                        crc >>= 1;
                    }
                }
            }
            return crc;
        }
    }
}
