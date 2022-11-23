using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.Collections.Specialized;
using System.Data;
using System.Globalization;
using System.Linq;
using System.Net.Sockets;
using System.Net;
using System.Text;
using System.Threading;
using System.Threading.Tasks;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Data;
using System.Windows.Documents;
using System.Windows.Input;
using System.Windows.Media;
using System.Windows.Media.Imaging;
using System.Windows.Navigation;
using System.Windows.Shapes;
using System.Reflection;

namespace SystoLockClient
{
    public partial class MainWindow : Window
    {
        Socket client;
        String encodingPassword = null;
        ObservableCollection<User> userdata = new ObservableCollection<User>();
        bool initialization = true;
        /// <summary>
        /// Window initialization.
        /// </summary>
        public MainWindow()
        {
            Thread.CurrentThread.CurrentUICulture = new CultureInfo("en-us");
            InitializeComponent();

            usersGrid.DataContext = userdata;
            userdata.CollectionChanged += AddingNewUser;
            usersGrid.CellEditEnding += EditUser;

            ConnectToServer();
            Closing += MainWindow_Closing;
        }

        /// <summary>
        /// Flushes database before window closing
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        async private void MainWindow_Closing(object sender, System.ComponentModel.CancelEventArgs e)
        {
            for (int i = 0; i < userdata.Count; i++)
            {
                EditUser(i);
            }
            String message = "End/";

            var messageBytes = Encoding.UTF8.GetBytes(message);
            _ = await client.SendAsync(new ArraySegment<byte>(messageBytes), SocketFlags.None);

            client.Shutdown(SocketShutdown.Both);
        }

        /// <summary>
        /// Establishes connection with the server
        /// </summary>
        async private void ConnectToServer()
        {
            usersGrid.Visibility = Visibility.Collapsed; 
            IPHostEntry host = Dns.GetHostEntry(Dns.GetHostName());
            bool hasntCrashed = false;

            for (int i = 0; i < host.AddressList.Length; i++)
            {
                IPEndPoint ipEndPoint = new(host.AddressList[i], 20248);
                client = new(ipEndPoint.AddressFamily, SocketType.Stream, ProtocolType.Tcp);
                try
                {
                    await client.ConnectAsync(ipEndPoint);
                    hasntCrashed = true;
                    break;
                }
                catch (Exception ex)
                {
                    continue;
                }
            }
            if (!hasntCrashed)
            {
                InputDialogSample inputDialog = new InputDialogSample("Could not connect to the server: none of localhost addresslist are suited for connection");
                inputDialog.ShowDialog();
                System.Windows.Application.Current.Shutdown();
            }
            usersGrid.Visibility = Visibility.Visible;  
            txtConnection.Visibility = Visibility.Collapsed;
            var buffer = new byte[1024];
            var received = await client.ReceiveAsync(new ArraySegment<byte>(buffer), SocketFlags.None);
            var response = Encoding.UTF8.GetString(buffer, 0, received);

            if (response == "Provide password to decrypt existing data.")
            {
                Boolean provided = false;
                do
                {
                    InputDialogSample inputDialog = new InputDialogSample("Please enter password to decrypt database with:");
                    usersGrid.Visibility = Visibility.Hidden;
                    if (inputDialog.ShowDialog() == true)
                    {
                        if (inputDialog.Answer == "") continue;
                        encodingPassword = inputDialog.Answer;
                        provided = true;
                    }
                } while (!provided);
                usersGrid.Visibility = Visibility.Visible;   
                var messageBytes = Encoding.UTF8.GetBytes(encodingPassword);
                _ = await client.SendAsync(new ArraySegment<byte>(messageBytes), SocketFlags.None);
                await RequestDatabase();
            }
            initialization = false;
        }

        /// <summary>
        /// Reads database sent from server
        /// </summary>
        /// <returns></returns>
        async private Task RequestDatabase()
        {
            var buffer = new byte[1024];
            String totalResponse = "";
            var received = await client.ReceiveAsync(new ArraySegment<byte>(buffer), SocketFlags.None);
            var response = Encoding.UTF8.GetString(buffer, 0, received);
            int entriesAm = int.Parse(response.Substring(0, response.IndexOf('/')));
            totalResponse = response;
            while(totalResponse.Count(f => f == '/') != entriesAm + 1)
            {
                received = await client.ReceiveAsync(new ArraySegment<byte>(buffer), SocketFlags.None);
                response = Encoding.UTF8.GetString(buffer, 0, received);
                totalResponse += response;
            }
            String[] parts = totalResponse.Split('/');
            for (int i = 1; i < parts.Length - 1; i++)
            {
                User newUser = new User();
                var userParts = parts[i].Split('|');
                newUser.firstName = userParts[0];
                newUser.lastName = userParts[1];
                newUser.initials= userParts[2];
                newUser.displayName = userParts[3];
                newUser.description = userParts[4]; 
                newUser.office = userParts[5];  
                newUser.telephoneNumber = userParts[6];
                newUser.EMail = userParts[7];
                newUser.webPage = userParts[8];
                newUser.creationDate = userParts[9];
                userdata.Add(newUser);
            }
        }

        /// <summary>
        /// Reacts to event changing the users collection
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        async private void AddingNewUser(object sender, NotifyCollectionChangedEventArgs e)
        {
            if (initialization) return;
            if (e.Action == NotifyCollectionChangedAction.Remove)
            {
                RemoveRow(e.OldStartingIndex);
                return;
            }
            InputDialogSample inputDialog;
            byte[] messageBytes;
            if (encodingPassword == null)
            {
                inputDialog = new InputDialogSample("Please enter password to encrypt database with:");
                if (inputDialog.ShowDialog() == true)
                {
                    ((User)(e.NewItems[0])).creationDate = DateTime.Now.ToString("yyyy-M-dd hh:mm:ss");
                    encodingPassword = inputDialog.Answer;
                    if (encodingPassword == "")
                    {
                        userdata.Clear();
                        return;
                    }
                    messageBytes = Encoding.UTF8.GetBytes(encodingPassword);
                    _ = await client.SendAsync(new ArraySegment<byte>(messageBytes), SocketFlags.None);
                }
                else
                {
                    userdata.Clear();
                    return;
                }
            }
            ((User)(e.NewItems[0])).creationDate = DateTime.Now.ToString("yyyy-M-dd hh:mm:ss");
             
            String message = "New|";
            User user = (User)(e.NewItems[0]);
            message += user.firstName + "|";
            message += user.lastName + "|";
            message += user.initials + "|";
            message += user.displayName + "|";
            message += user.description + "|";
            message += user.office + "|";
            message += user.telephoneNumber + "|";
            message += user.EMail + "|";
            message += user.webPage + "|";
            message += user.creationDate + "/";

            messageBytes = Encoding.UTF8.GetBytes(message);
            _ = await client.SendAsync(new ArraySegment<byte>(messageBytes), SocketFlags.None);
            EditUser(userdata.Count - 2);
        }

        /// <summary>
        /// Sends command to remove user from database
        /// </summary>
        /// <param name="index"></param>
        async private void RemoveRow(int index)
        { 
            String message = "Delete|";
            message += index.ToString() + "/";

            var messageBytes = Encoding.UTF8.GetBytes(message);
            _ = await client.SendAsync(new ArraySegment<byte>(messageBytes), SocketFlags.None);
        }

        /// <summary>
        /// Sends command to edit user in a database
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        async void EditUser(object sender, DataGridCellEditEndingEventArgs e)
        {
            int rowIndex = e.Row.GetIndex();
            EditUser(rowIndex);
        }

        /// <summary>
        /// Sends command to edit user in a database
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        async void EditUser(int rowIndex)
        {
            if (initialization) return;
            if (rowIndex < 0) return;
            String message = "Edit|";
            User user = (User)(userdata[rowIndex]);
            message += rowIndex.ToString() + "|";
            message += user.firstName + "|";
            message += user.lastName + "|";
            message += user.initials + "|";
            message += user.displayName + "|";
            message += user.description + "|";
            message += user.office + "|";
            message += user.telephoneNumber + "|";
            message += user.EMail + "|";
            message += user.webPage + "|";
            message += user.creationDate + "/";

            var messageBytes = Encoding.UTF8.GetBytes(message);
            _ = await client.SendAsync(new ArraySegment<byte>(messageBytes), SocketFlags.None);
        }
    }
}
