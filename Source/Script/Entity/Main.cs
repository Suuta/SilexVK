
using System;
using System.Runtime.CompilerServices;

namespace Silex
{
    public struct Vector3
    {
        public float x, y, z;

        public Vector3(float x, float y, float z)
        { 
            this.x = x;
            this.y = y;
            this.z = z;
        }
    }

    public static class InternalCall
    {
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static void NativeLog_String(string text);

        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static void NativeLog_Vector3(ref Vector3 param, out Vector3 outParam);
    }

    public class Main
    {
        public float floatValue { get; set;}

        public Main()
        {
            Console.WriteLine("Main コンストラクター");
            InternalCall.NativeLog_String("NativeLog");

            Vector3 up = new Vector3(1, 2, 1);

            Vector3 result = new Vector3();
            InternalCall.NativeLog_Vector3(ref up, out result);

            Console.WriteLine("result = {0}, {1}, {2}", result.x, result.y, result.z);
        }

        public void Print()
        {
            Console.WriteLine("Main.Print");
        }

        public void Print(int value)
        {
            Console.WriteLine("Main.PrintInt: {0}", value);
        }

        public void Print(string message)
        {
            Console.WriteLine("Main.Print: {0}", message);
        }
    }
}
