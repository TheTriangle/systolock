   M S B A M L   `   `   `  ????J  FSystoLockClient, Version=1.0.0.0, Culture=neutral, PublicKeyToken=null1clr-namespace:SystoLockClientSystoLockClient  5   	        SystoLockClient.MainWindow   5   	   R NWindowsBase, Version=4.0.0.0, Culture=neutral, PublicKeyToken=31bf3856ad364e35\ XPresentationFramework, Version=4.0.0.0, Culture=neutral, PublicKeyToken=31bf3856ad364e35W SPresentationCore, Version=4.0.0.0, Culture=neutral, PublicKeyToken=31bf3856ad364e35D 9http://schemas.microsoft.com/winfx/2006/xaml/presentation    5   	   R NSystem.Xaml, Version=4.0.0.0, Culture=neutral, PublicKeyToken=b77a5c561934e089:x,http://schemas.microsoft.com/winfx/2006/xaml     5   	   @d2http://schemas.microsoft.com/expression/blend/2008     5   	   Jmc;http://schemas.openxmlformats.org/markup-compatibility/2006     5   	   /localclr-namespace:SystoLockClient     5   	     ? Title$  
MainWindow??5   	   $	??450??6   $	??800??6)   .??5	      ?? .+?5
   
   ?? -   5         ??Name$ txtConnection??5
      $0??*Please wait while we connect to the server??6'   ? 5   
   
???????.+?5      &   System.Windows.Controls.DataGrid  -   5          Name$ 	usersGrid??5         CanUserAddRows .    CanUserDeleteRows .    AutoGenerateColumns .     CanUserSortColumns .     CanUserReorderColumns .  ??6*   ??    Columns 5      0  *System.Windows.Controls.DataGridTextColumn  5      ,  &System.Windows.Controls.DataGridColumn	   Header$	 
First Name??6)   1  +System.Windows.Controls.DataGridBoundColumn
   Binding
 6>   ?? *	firstName+ ?? UpdateSourceTrigger$ 	LostFocus=?  5      $	 	Last Name??6)   
 6<   ?? *
lastName+$ 	LostFocus=?  5      $	 Initials??6)   
 6;   ?? *
initials+$ 	LostFocus=?  5      $	 Display Name??6)   
 6?   ?? *displayName+$ 	LostFocus=?  5      $	 Description??6)   
 6>   ?? *description+$ 	LostFocus=?  5      $	 Office??6)   
 69   ?? *office+$ 	LostFocus=?  5      $	 Phone Number??6)   
 6?   ?? *telephoneNumber+$ 	LostFocus=?  5      $	 Email??6)   
 68   ?? *EMail+$ 	LostFocus=?  5      $	 Webpage??6)   
 6:   ?? *	webPage+$ 	LostFocus=?  5      $	 Creation date??6)      
IsReadOnly . 
 6@   ?? *creationDate+$ 	LostFocus=?6   5      5      5      5      